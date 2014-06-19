/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "swapping_gl_context.h"
#include "hwc_device.h"
#include "hwc_layerlist.h"
#include "hwc_vsync_coordinator.h"
#include "hwc_wrapper.h"
#include "framebuffer_bundle.h"
#include "buffer.h"
#include "hwc_fallback_gl_renderer.h"
#include <limits>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom = mir::geometry;

namespace
{
static const size_t fbtarget_plus_skip_size = 2;
static const size_t fbtarget_size = 1;

bool plane_alpha_is_translucent(mg::Renderable const& renderable)
{
    float static const tolerance
    {
        1.0f/(2.0 * static_cast<float>(std::numeric_limits<decltype(hwc_layer_1_t::planeAlpha)>::max()))
    };
    return renderable.alpha_enabled() && (renderable.alpha() < 1.0f - tolerance);
}

bool renderable_list_is_hwc_incompatible(mg::RenderableList const& list)
{
    if (list.empty())
        return true;

    for(auto const& renderable : list)
    {
        //TODO: enable planeAlpha for (hwc version >= 1.2), 90 deg rotation
        static glm::mat4 const identity;
        if (plane_alpha_is_translucent(*renderable) ||
           (renderable->transformation() != identity))
        {
            return true;
        }
    }
    return false;
}
}

mga::HwcDevice::HwcDevice(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                          std::shared_ptr<HwcWrapper> const& hwc_wrapper,
                          std::shared_ptr<HWCVsyncCoordinator> const& coordinator,
                          std::shared_ptr<SyncFileOps> const& sync_ops)
    : HWCCommonDevice(hwc_device, coordinator),
      hwc_list{{}, fbtarget_plus_skip_size},
      hwc_wrapper(hwc_wrapper), 
      sync_ops(sync_ops)
{
}

bool mga::HwcDevice::post_overlays(
    SwappingGLContext const& context,
    RenderableList const& renderables,
    RenderableListCompositor const& list_compositor)
{
    printf("ooo\n");
    if (renderable_list_is_hwc_incompatible(renderables))
    {
        printf("???\n");
        return false;
    }
    if (!hwc_list.update_list_and_check_if_changed(renderables, fbtarget_size))
    {
        printf("reject.\n");
        return false;
    }

    printf("gothere\n");
    auto lg = lock_unblanked();

    auto& buffer = *context.last_rendered_buffer();
    geom::Rectangle const disp_frame{{0,0}, {buffer.size()}};
    auto& fbtarget_layer = *hwc_list.additional_layers_begin();
    fbtarget_layer.setup_layer(mga::LayerType::framebuffer_target, disp_frame, false, buffer);

    printf("PREP!\n");
    hwc_wrapper->prepare(*hwc_list.native_list().lock());

    (void) list_compositor;
    (void)context;
    mg::RenderableList rejected_renderables;
    std::vector<std::shared_ptr<mg::Buffer>> next_onscreen_overlay_buffers;

    auto layers_it = hwc_list.begin();
    for(auto const& renderable : renderables)
    {
        layers_it->prepare_for_draw(*renderable->buffer());
        if (layers_it->needs_gl_render())
            rejected_renderables.push_back(renderable);
        else
            next_onscreen_overlay_buffers.push_back(renderable->buffer());
        layers_it++;
    }

    list_compositor.render(rejected_renderables, context);

    buffer = *context.last_rendered_buffer();
    fbtarget_layer.setup_layer(mga::LayerType::framebuffer_target, disp_frame, false, buffer);
    fbtarget_layer.prepare_for_draw(buffer);

    hwc_wrapper->set(*hwc_list.native_list().lock());
    onscreen_overlay_buffers = std::move(next_onscreen_overlay_buffers);

    layers_it = hwc_list.begin();
    for(auto const& renderable : renderables)
    {
        layers_it->update_fence(*renderable->buffer());
        layers_it++;
    }
    printf("update?\n");
    fbtarget_layer.update_fence(buffer);

    mga::SyncFence retire_fence(sync_ops, hwc_list.retirement_fence());
    return true;
}

void mga::HwcDevice::post_gl(SwappingGLContext const& context)
{
    auto lg = lock_unblanked();
    hwc_list.update_list_and_check_if_changed({}, fbtarget_plus_skip_size);

    auto& buffer = *context.last_rendered_buffer();
    geom::Rectangle const disp_frame{{0,0}, {buffer.size()}};
    auto& skip_layer = *hwc_list.additional_layers_begin();
    auto& fb_layer = *(++hwc_list.additional_layers_begin());
    skip_layer.setup_layer(mga::LayerType::skip, disp_frame, false, buffer);
    fb_layer.setup_layer(mga::LayerType::framebuffer_target, disp_frame, false, buffer);

    hwc_wrapper->prepare(*hwc_list.native_list().lock());

    context.swap_buffers();

    buffer = *context.last_rendered_buffer();
    skip_layer.setup_layer(mga::LayerType::skip, disp_frame, false, buffer);
    fb_layer.setup_layer(mga::LayerType::framebuffer_target, disp_frame, false, buffer);

    for(auto& layer : hwc_list)
        layer.prepare_for_draw(buffer);

    hwc_wrapper->set(*hwc_list.native_list().lock());
    onscreen_overlay_buffers.clear();

    for(auto& layer : hwc_list)
        layer.update_fence(buffer);

    mga::SyncFence retire_fence(sync_ops, hwc_list.retirement_fence());
}
