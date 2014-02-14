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
                          std::shared_ptr<HWCVsyncCoordinator> const& coordinator,
                          std::shared_ptr<SyncFileOps> const& sync_ops)
    : HWCCommonDevice(hwc_device, coordinator),
      sync_ops(sync_ops)
{
}

void mga::HwcDevice::prepare()
{
    auto rc = 0;
    auto display_list = layer_list.native_list().lock();
    if (display_list)
    {
        //note, although we only have a primary display right now,
        //      set the external and virtual displays to null as some drivers check for that
        hwc_display_contents_1_t* displays[num_displays] {display_list.get(), nullptr, nullptr};
        rc = hwc_device->prepare(hwc_device.get(), 1, displays);
    }

    if ((rc != 0) || (!display_list))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error during hwc prepare()"));
    }
}

void mga::HwcDevice::prepare_gl()
{
    layer_list.reset_composition_layers();
    prepare();
}

void mga::HwcDevice::prepare_gl_and_overlays(
    std::list<std::shared_ptr<Renderable>> const& renderables,
    std::function<void(Renderable const&)> const& render_fn)
{
    layer_list.set_composition_layers(renderables);
    prepare();
    //TODO: filter this list based on the results of the preparation
    for(auto const& renderable : renderables)
        render_fn(*renderable);
}

void mga::HwcDevice::gpu_render(EGLDisplay dpy, EGLSurface sur)
{
    if (eglSwapBuffers(dpy, sur) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("eglSwapBuffers failure\n"));
    }
}

void mga::HwcDevice::post(mg::Buffer const& buffer)
{
    auto lg = lock_unblanked();

    layer_list.set_fb_target(buffer);

    auto rc = 0;
    auto display_list = layer_list.native_list().lock();
    if (display_list)
    {
        hwc_display_contents_1_t* displays[num_displays] {display_list.get(), nullptr, nullptr};
        rc = hwc_device->set(hwc_device.get(), 1, displays);

        layer_list.update_fences();
        mga::SyncFence retire_fence(sync_ops, layer_list.retirement_fence());
    }

    if ((rc != 0) || (!display_list))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error during hwc set()"));
    }
}
