/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/buffer.h"
#include "mir/graphics/android/sync_fence.h"
#include "mir/graphics/android/native_buffer.h"
#include "android_format_conversion-inl.h"
#include "fb_device.h"
#include "framebuffer_bundle.h"
#include "buffer.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::FBDevice::FBDevice(
    std::shared_ptr<framebuffer_device_t> const& fbdev,
    std::shared_ptr<FramebufferBundle> const& bundle)
    : fb_device(fbdev),
      fb_bundle(bundle)
{
    if (fb_device->setSwapInterval)
    {
        fb_device->setSwapInterval(fb_device.get(), 1);
    }
}

void mga::FBDevice::set_next_frontbuffer(std::shared_ptr<mg::Buffer> const&)
{
}

geom::Size mga::FBDevice::display_size() const
{
    return {fb_device->width, fb_device->height};
} 

geom::PixelFormat mga::FBDevice::display_format() const
{
    return to_mir_format(fb_device->format);
}

std::shared_ptr<mg::Buffer> mga::FBDevice::buffer_for_render()
{
    return fb_bundle->buffer_for_render();
}

void mga::FBDevice::sync_to_display(bool sync)
{
    if (!fb_device->setSwapInterval)
        return;

    if (sync)
    {
        fb_device->setSwapInterval(fb_device.get(), 1);
    }
    else
    {
        fb_device->setSwapInterval(fb_device.get(), 0);
    }
}

void mga::FBDevice::commit_frame(EGLDisplay dpy, EGLSurface sur)
{
    if (eglSwapBuffers(dpy, sur) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("eglSwapBuffers failure\n"));
    }

    auto buffer = fb_bundle->last_rendered_buffer();
    auto native_buffer = buffer->native_buffer_handle();
    native_buffer->wait_for_content();

    if (fb_device->post(fb_device.get(), native_buffer->handle()) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error posting with fb device"));
    }
}

void mga::FBDevice::mode(MirPowerMode mode)
{
    // TODO: Implement
    (void) mode;
}
