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

#include <thread>
#include <chrono>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom = mir::geometry;

mga::HwcDevice::HwcDevice(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                          std::shared_ptr<HwcWrapper> const& hwc_wrapper,
                          std::shared_ptr<HWCVsyncCoordinator> const& coordinator,
                          std::shared_ptr<SyncFileOps> const& sync_ops)
    : HWCCommonDevice(hwc_device, coordinator), 
      LayerListBase{2},
      hwc_wrapper(hwc_wrapper), 
      sync_ops(sync_ops)
{
}

void mga::HwcDevice::render_gl(SwappingGLContext const& context)
{
    printf("prepping.\n");
    update_representation(2, {});
    layers.front().set_layer_type(mga::LayerType::skip);
    layers.back().set_layer_type(mga::LayerType::framebuffer_target);
    list_needs_commit = true;
    skip_layers_present = true;

    hwc_wrapper->prepare(*native_list().lock());

    context.swap_buffers();
}

void mga::HwcDevice::render_gl_and_overlays(
    SwappingGLContext const& context,
    std::list<std::shared_ptr<Renderable>> const& renderables,
    std::function<void(Renderable const&)> const& render_fn)
{
    auto const needed_size = renderables.size() + 1;
    update_representation(needed_size, renderables);
    if (!list_has_changed())
        return;

    list_needs_commit = true;
    skip_layers_present = false;

    layers.back().set_layer_type(mga::LayerType::framebuffer_target);
    hwc_wrapper->prepare(*native_list().lock());

    //draw layers that the HWC did not accept for overlays here
    bool needs_swapbuffers = false;
    auto layers_it = layers.begin();
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

    //swap if needed
    if (needs_swapbuffers)
        context.swap_buffers(); 
}

void mga::HwcDevice::post(mg::Buffer const& buffer)
{
    if (!list_needs_commit)
    {
        printf("ok.\n");
        return;
    }

    printf("actually committing\n");
    auto lg = lock_unblanked();

    geom::Rectangle const disp_frame{{0,0}, {buffer.size()}};
    //TODO: the functions on mga::Layer should be consolidated
    if (skip_layers_present)
    {
        layers.front().set_render_parameters(disp_frame, false);
        layers.front().set_buffer(buffer);
        layers.front().prepare_for_draw();
    }

    layers.back().set_render_parameters(disp_frame, false);
    layers.back().set_buffer(buffer);
    layers.back().prepare_for_draw();

    hwc_wrapper->set(*native_list().lock());

    for(auto& layer : layers)
        layer.update_fence_and_release_buffer();

    //On some drivers, this fence seems unreliable for display timing purposes. make sure to close()
    mga::SyncFence retire_fence(sync_ops, retirement_fence());
    list_needs_commit = false;
}
