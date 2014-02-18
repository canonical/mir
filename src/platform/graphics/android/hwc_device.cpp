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

#include "hwc_device.h"
#include "hwc_layerlist.h"
#include "hwc_vsync_coordinator.h"
#include "framebuffer_bundle.h"
#include "buffer.h"
#include "mir/graphics/buffer.h"

#include <EGL/eglext.h>
#include <boost/throw_exception.hpp>
#include <stdexcept>

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
      sync_ops(sync_ops),
      needs_swapbuffers{true}
{
    layers.front().set_layer_type(mga::LayerType::skip);
    layers.back().set_layer_type(mga::LayerType::framebuffer_target);
}

void mga::HwcDevice::prepare_gl()
{
    update_representation(2);
    layers.front().set_layer_type(mga::LayerType::skip);
    layers.back().set_layer_type(mga::LayerType::framebuffer_target);
    skip_layers_present = true;

    hwc_wrapper->prepare(*native_list().lock());

    needs_swapbuffers = true;
}

void mga::HwcDevice::prepare_gl_and_overlays(
    std::list<std::shared_ptr<Renderable>> const& renderables,
    std::function<void(mg::Renderable const&)> const& render_fn) 
{
    auto const needed_size = renderables.size() + 1;
    update_representation(needed_size);

    //pack layer list from renderables
    auto layers_it = layers.begin();
    for(auto const& renderable : renderables)
    {
        layers_it->set_layer_type(mga::LayerType::gl_rendered);
        layers_it->set_render_parameters(renderable->screen_position(), renderable->alpha_enabled());
        layers_it->set_buffer(renderable->buffer()->native_buffer_handle());
        layers_it++;
    }
    layers_it->set_layer_type(mga::LayerType::framebuffer_target);
    skip_layers_present = false;

    hwc_wrapper->prepare(*native_list().lock());

    //if a layer cannot be drawn, draw with GL here
    layers_it = layers.begin();
    bool gl_render_needed = false;
    for(auto const& renderable : renderables)
    {
        if ((layers_it++)->needs_gl_render())
        {
            gl_render_needed = true;
            render_fn(*renderable);
        }
    }
    needs_swapbuffers = gl_render_needed; 
}

void mga::HwcDevice::gpu_render(EGLDisplay dpy, EGLSurface sur)
{
    if ((needs_swapbuffers) && 
        (eglSwapBuffers(dpy, sur) == EGL_FALSE))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("eglSwapBuffers failure\n"));
    }
}

void mga::HwcDevice::post(mg::Buffer const& buffer)
{
    auto lg = lock_unblanked();

    geom::Rectangle const disp_frame{{0,0}, {buffer.size()}};
    auto buf = buffer.native_buffer_handle();
    if (skip_layers_present)
    {
        layers.front().set_render_parameters(disp_frame, false);
        layers.front().set_buffer(buf);
    }

    layers.back().set_render_parameters(disp_frame, false);
    layers.back().set_buffer(buf);

    hwc_wrapper->set(*native_list().lock());

    for(auto& layer : layers)
    {
        layer.update_fence_and_release_buffer();
    }
    mga::SyncFence retire_fence(sync_ops, retirement_fence());
}
