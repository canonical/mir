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

#include "display_buffer.h"
#include "display_device.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

namespace
{
static EGLint const egl_context_attr[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};
}

mga::DisplayBuffer::DisplayBuffer(std::shared_ptr<DisplayDevice> const& display_device,
    std::shared_ptr<ANativeWindow> const& native_window,
    EGLDisplay connection, EGLConfig config, EGLContext shared_context)
    : display_device{display_device},
      native_window{native_window},
      egl_display{connection},
      egl_context{
          connection,
          eglCreateContext(connection, config, shared_context, egl_context_attr)},
      egl_surface{
          connection,
          eglCreateWindowSurface(connection, config, native_window.get(), NULL)}
{
}

geom::Rectangle mga::DisplayBuffer::view_area() const
{
    return {geom::Point{}, display_device->display_size()};
}

void mga::DisplayBuffer::make_current()
{
    if (eglMakeCurrent(egl_display, egl_surface,
                       egl_surface, egl_context) == EGL_FALSE)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("could not activate surface with eglMakeCurrent\n"));
    }
}

void mga::DisplayBuffer::release_current()
{
    eglMakeCurrent(egl_display, EGL_NO_SURFACE,
                   EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void mga::DisplayBuffer::post_update()
{
    display_device->commit_frame(egl_display, egl_surface);
}

bool mga::DisplayBuffer::can_bypass() const
{
    return false;
}
