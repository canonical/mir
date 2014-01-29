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

#include "hwc_fb_device.h"
#include "hwc_layers.h"
#include "hwc_vsync_coordinator.h"
#include "framebuffer_bundle.h"
#include "android_format_conversion-inl.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/android/native_buffer.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

mga::HwcFbDevice::HwcFbDevice(std::shared_ptr<hwc_composer_device_1> const& hwc_device,
                              std::shared_ptr<framebuffer_device_t> const& fb_device,
                              std::shared_ptr<HWCVsyncCoordinator> const& coordinator)
    : HWCCommonDevice(hwc_device, coordinator),
      fb_device(fb_device),
      layer_list({mga::ForceGLLayer{}})
{
}

void mga::HwcFbDevice::prepare_gl()
{
    auto display_list = layer_list.native_list();
    if (hwc_device->prepare(hwc_device.get(), 1, &display_list) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error during hwc prepare()"));
    }
}

void mga::HwcFbDevice::prepare_gl_and_overlays(std::list<std::shared_ptr<Renderable>> const&)
{
    prepare_gl();
}

void mga::HwcFbDevice::gpu_render(EGLDisplay dpy, EGLSurface sur)
{
    auto display_list = layer_list.native_list();
    display_list->dpy = dpy;
    display_list->sur = sur;

    //set() may affect EGL state by calling eglSwapBuffers.
    //HWC 1.0 is the only version of HWC that can do this.
    if (hwc_device->set(hwc_device.get(), 1, &display_list) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error during hwc set()"));
    }
}

void mga::HwcFbDevice::post(mg::Buffer const& buffer)
{
    auto lg = lock_unblanked();

    auto native_buffer = buffer.native_buffer_handle();
    native_buffer->wait_for_content();
    if (fb_device->post(fb_device.get(), native_buffer->handle()) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error posting with fb device"));
    }

    coordinator->wait_for_vsync();
}
