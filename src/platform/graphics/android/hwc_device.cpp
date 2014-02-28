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

#include "gl_context.h"
#include "hwc_device.h"
#include "hwc_layerlist.h"
#include "hwc_vsync_coordinator.h"
#include "framebuffer_bundle.h"
#include "buffer.h"
#include "mir/graphics/buffer.h"

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom = mir::geometry;

mga::HwcDevice::HwcDevice(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                          std::shared_ptr<HwcWrapper> const& hwc_wrapper,
                          std::shared_ptr<HWCVsyncCoordinator> const& coordinator,
                          std::shared_ptr<SyncFileOps> const& sync_ops)
    : HWCCommonDevice(hwc_device, coordinator),
      hwc_list{{}, 2},
      hwc_wrapper(hwc_wrapper), 
      sync_ops(sync_ops)
{
    auto it = hwc_list.additional_layers_begin();
    it++->set_layer_type(mga::LayerType::skip);
    it++->set_layer_type(mga::LayerType::framebuffer_target);
}

void mga::HwcDevice::render_gl(SwappingGLContext const& context)
{
    hwc_list.update_list_and_check_if_changed({}, 2);
    auto it = hwc_list.additional_layers_begin();
    it++->set_layer_type(mga::LayerType::skip);
    it++->set_layer_type(mga::LayerType::framebuffer_target);

    list_needs_commit = true;
    skip_layers_present = true;

    hwc_wrapper->prepare(*hwc_list.native_list().lock());

    context.swap_buffers();
}

void mga::HwcDevice::render_gl_and_overlays(
    SwappingGLContext const& context,
    std::list<std::shared_ptr<Renderable>> const& renderables,
    std::function<void(Renderable const&)> const& render_fn)
{
    auto const additional_layers = 1; //1 extra layer for FBTARGET
    if (!(list_needs_commit = hwc_list.update_list_and_check_if_changed(renderables, additional_layers)))
        return;

    skip_layers_present = false;
    hwc_list.additional_layers_begin()->set_layer_type(mga::LayerType::framebuffer_target);

    hwc_wrapper->prepare(*hwc_list.native_list().lock());

    //draw layers that the HWC did not accept for overlays here
    bool needs_swapbuffers = false;
    auto layers_it = hwc_list.begin();
    for(auto const& renderable : renderables)
    {
        //prepare all layers for draw. 
        layers_it->prepare_for_draw();

        //trigger GL on the layers that are not overlays
        if (layers_it->needs_gl_render())
        {
            render_fn(*renderable);
            needs_swapbuffers = true;
        }
        layers_it++;
    }

    if (needs_swapbuffers)
        context.swap_buffers();
}

void mga::HwcDevice::post(mg::Buffer const& buffer)
{
    if (!list_needs_commit)
        return;

    auto lg = lock_unblanked();

    //TODO: the functions on mga::Layer should be consolidated
    geom::Rectangle const disp_frame{{0,0}, {buffer.size()}};
    auto it = hwc_list.additional_layers_begin();
    if (skip_layers_present)
    {
        it->set_render_parameters(disp_frame, false);
        it->set_buffer(buffer);
        it->prepare_for_draw();
        it++;
    }

    it->set_render_parameters(disp_frame, false);
    it->set_buffer(buffer);
    it->prepare_for_draw();

    hwc_wrapper->set(*hwc_list.native_list().lock());

    for(auto& layer : hwc_list)
    {
        layer.update_fence_and_release_buffer();
    }

    mga::SyncFence retire_fence(sync_ops, hwc_list.retirement_fence());
    list_needs_commit = false;
}
