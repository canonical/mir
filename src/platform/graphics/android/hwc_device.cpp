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

void mga::HwcDevice::setup_layer_types()
{
    auto it = hwc_list.additional_layers_begin();
    auto const num_additional_layers = std::distance(it, hwc_list.end());
    switch (num_additional_layers)
    {
        case fbtarget_plus_skip_size:
            it->set_layer_type(mga::LayerType::skip);
            ++it;
        case fbtarget_size:
            it->set_layer_type(mga::LayerType::framebuffer_target);
        default:
            break;
    }
}

void mga::HwcDevice::set_list_framebuffer(mg::Buffer const& buffer)
{
    geom::Rectangle const disp_frame{{0,0}, {buffer.size()}};
    for(auto it = hwc_list.additional_layers_begin(); it != hwc_list.end(); it++)
    {
        //TODO: the functions on mga::Layer should be consolidated
        it->set_render_parameters(disp_frame, false);
        it->set_buffer(buffer);
        it->prepare_for_draw();
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
    setup_layer_types();
}

void mga::HwcDevice::post_gl(SwappingGLContext const& context)
{
    hwc_list.update_list_and_check_if_changed({}, fbtarget_plus_skip_size);
    setup_layer_types();

    hwc_wrapper->prepare(*hwc_list.native_list().lock());

    context.swap_buffers();

    post(context);
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
    setup_layer_types();

    hwc_wrapper->prepare(*hwc_list.native_list().lock());

    mg::RenderableList rejected_renderables;

    std::vector<std::shared_ptr<mg::Buffer>> next_onscreen_overlay_buffers;
    auto layers_it = hwc_list.begin();
    for(auto const& renderable : renderables)
    {
        layers_it->prepare_for_draw();
        if (layers_it->needs_gl_render())
            rejected_renderables.push_back(renderable);
        else
            next_onscreen_overlay_buffers.push_back(renderable->buffer());
        layers_it++;
    }

    list_compositor.render(rejected_renderables, context);
    post(context);
    onscreen_overlay_buffers = std::move(next_onscreen_overlay_buffers);
    return true;
}

void mga::HwcDevice::post(SwappingGLContext const& context)
{
    auto lg = lock_unblanked();
    set_list_framebuffer(*context.last_rendered_buffer());
    hwc_wrapper->set(*hwc_list.native_list().lock());

    for(auto& layer : hwc_list)
        layer.update_fence_and_release_buffer();

    mga::SyncFence retire_fence(sync_ops, hwc_list.retirement_fence());
}
