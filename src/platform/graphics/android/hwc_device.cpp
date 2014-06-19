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

void mga::HwcDevice::setup_layer_types(mg::Buffer const& buffer)
{
    geom::Rectangle const disp_frame{{0,0}, {buffer.size()}};
    auto it = hwc_list.additional_layers_begin();
    auto const num_additional_layers = std::distance(it, hwc_list.end());
    switch (num_additional_layers)
    {
        case fbtarget_plus_skip_size:
            it->setup_layer(
                mga::LayerType::skip,
                disp_frame,
                false,
                buffer);
            ++it;
        case fbtarget_size:
            it->setup_layer(
                mga::LayerType::framebuffer_target,
                disp_frame,
                false,
                buffer);
        default:
            break;
    }
}

void mga::HwcDevice::set_list_framebuffer(mg::Buffer const& buffer)
{
    for(auto it = hwc_list.additional_layers_begin(); it != hwc_list.end(); it++)
    {
        it->prepare_for_draw(buffer);
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
    printf("a\n");
    hwc_list.update_list_and_check_if_changed({}, fbtarget_plus_skip_size);
    printf("b\n");
    setup_layer_types(*context.last_rendered_buffer());
    printf("c\n");

    hwc_wrapper->prepare(*hwc_list.native_list().lock());

    context.swap_buffers();

    setup_layer_types(*context.last_rendered_buffer());
    set_list_framebuffer(*context.last_rendered_buffer());
    post();
    for(auto& layer : hwc_list)
        layer.update_fence_and_release_buffer(*context.last_rendered_buffer());
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
    {
        printf("no\n");
        return false;
    }
    setup_layer_types(*context.last_rendered_buffer());

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
    setup_layer_types(*context.last_rendered_buffer());
    set_list_framebuffer(*context.last_rendered_buffer());

    post();

    layers_it = hwc_list.begin();
    for(auto const& renderable : renderables)
    {
        layers_it->update_fence_and_release_buffer(*renderable->buffer());
        layers_it++;
    }

    onscreen_overlay_buffers = std::move(next_onscreen_overlay_buffers);
    return true;
}

void mga::HwcDevice::post()
{
    auto lg = lock_unblanked();
    hwc_wrapper->set(*hwc_list.native_list().lock());
    mga::SyncFence retire_fence(sync_ops, hwc_list.retirement_fence());
}
