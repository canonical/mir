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

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom = mir::geometry;

namespace
{
static const size_t fbtarget_plus_skip_size = 2;
static const size_t fbtarget_size = 1;
bool renderable_list_is_hwc_incompatible(mg::RenderableList const& list)
{
    if (list.empty())
        return true;

    for(auto const& renderable : list)
    {
        //TODO: enable alpha, 90 deg rotation
        static glm::mat4 const identity;
        if (renderable->shaped() ||
            renderable->alpha_enabled() ||
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
      hwc_list{{}, 2},
      hwc_wrapper(hwc_wrapper), 
      sync_ops(sync_ops)
{
}

void mga::HwcDevice::post_gl(SwappingGLContext const& context)
{
    auto lg = lock_unblanked();
    hwc_list.update_list_and_check_if_changed({}, fbtarget_plus_skip_size);
 
    geom::Rectangle const disp_frame{{0,0}, {prebuffer->size()}};
    auto const& prebuffer = context.last_rendered_buffer();
    auto& skip_layer = *hwc_list.additional_layers_begin();
    auto& fbtarget_layer = *(hwc_list.additional_layers_begin()++);
    skip_layer.setup_layer(mga::LayerType::skip, disp_frame, false, *prebuffer);
    fbtarget_layer.setup_layer(mga::LayerType::framebuffer_target, disp_frame, false, *prebuffer);

    hwc_wrapper->prepare(*hwc_list.native_list().lock());

    context.swap_buffers();

    auto const& buffer = context.last_rendered_buffer();
    skip_layer.setup_layer(mga::LayerType::skip, disp_frame, false, *buffer);
    fbtarget_layer.setup_layer(mga::LayerType::framebuffer_target, disp_frame, false, *buffer);

    hwc_wrapper->set(*hwc_list.native_list().lock());
    for(auto& layer : hwc_list)
        layer.update_fence(*context.last_rendered_buffer());

    mga::SyncFence retire_fence(sync_ops, hwc_list.retirement_fence());
    onscreen_overlay_buffers.clear();
}

bool mga::HwcDevice::post_overlays(
    SwappingGLContext const& context,
    RenderableList const& renderables,
    RenderableListCompositor const& list_compositor)
{
    if (renderable_list_is_hwc_incompatible(renderables))
        return false;
    if (!hwc_list.update_list_and_check_if_changed(renderables, fbtarget_size))
        return false;

    auto lg = lock_unblanked();

    geom::Rectangle const disp_frame{{0,0}, {prebuffer->size()}};
    auto const& prebuffer = context.last_rendered_buffer();
    auto & fbtarget_layer = *hwc_list.additional_layers_begin();
    fbtarget_layer.setup_layer(mga::LayerType::framebuffer_target, disp_frame, false, *prebuffer);

    hwc_wrapper->prepare(*hwc_list.native_list().lock());

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

    auto const& final_fb = *context.last_rendered_buffer();
    fbtarget_layer.setup_layer(mga::LayerType::framebuffer_target, disp_frame, false, final_fb);
    fbtarget_layer.prepare_for_draw(final_fb);

    hwc_wrapper->set(*hwc_list.native_list().lock());

    //take care of releaseFenceFds
    layers_it = hwc_list.begin();
    for(auto const& renderable : renderables)
    {
        layers_it->update_fence(*renderable->buffer());
        layers_it++;
    }
    fbtarget_layer.update_fence(final_fb);
    mga::SyncFence retire_fence(sync_ops, hwc_list.retirement_fence());

    onscreen_overlay_buffers = std::move(next_onscreen_overlay_buffers);
    return true;
}
