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

#include "fb_device.h"
#include "buffer.h"
#include "android_format_conversion-inl.h"

#include <algorithm>
#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

mga::FBDevice::FBDevice(std::shared_ptr<framebuffer_device_t> const& fbdev)
    : fb_device(fbdev)
{
    if (fb_device->setSwapInterval)
    {
        fb_device->setSwapInterval(fb_device.get(), 1);
    }
}

void mga::FBDevice::set_next_frontbuffer(std::shared_ptr<mg::Buffer> const& buffer)
{
    auto native_buffer = buffer->native_buffer_handle();
    native_buffer->wait_for_content();

    if (fb_device->post(fb_device.get(), native_buffer->handle()) != 0)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("error posting with fb device"));
    }
}

geom::Size mga::FBDevice::display_size() const
{
    return {fb_device->width, fb_device->height};
} 

geom::PixelFormat mga::FBDevice::display_format() const
{
    return to_mir_format(fb_device->format);
}

unsigned int mga::FBDevice::number_of_framebuffers_available() const
{
    auto fb_num = static_cast<unsigned int>(fb_device->numFramebuffers);
    return std::max(2u, fb_num);
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
}

void mga::FBDevice::mode(MirPowerMode mode)
{
    // TODO: Implement
    (void) mode;
}
