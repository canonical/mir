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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "display_buffer_factory.h"
#include "display_device.h"

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/egl_resources.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

namespace
{

EGLContext create_context(EGLDisplay egl_display,
                          EGLConfig egl_config,
                          EGLContext egl_context_shared)
{
    static EGLint const default_egl_context_attr[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    return eglCreateContext(egl_display, egl_config, egl_context_shared,
                            default_egl_context_attr);
}

EGLSurface create_surface(EGLDisplay egl_display,
                          EGLConfig egl_config,
                          EGLNativeWindowType native_win)
{
    return eglCreateWindowSurface(egl_display, egl_config, native_win, NULL);
}

class GPUDisplayBuffer : public mg::DisplayBuffer
{
public:
    GPUDisplayBuffer(std::shared_ptr<ANativeWindow> const& native_window,
                     EGLDisplay egl_display,
                     EGLConfig egl_config,
                     EGLContext egl_context_shared,
                     std::shared_ptr<mga::DisplayDevice> const& display_device)
        : native_window{native_window},
          display_device{display_device},
          egl_display{egl_display},
          egl_config{egl_config},
          egl_context{egl_display, create_context(egl_display, egl_config, egl_context_shared)},
          egl_surface{egl_display, create_surface(egl_display, egl_config, native_window.get())},
          blanked(false)
    {
    }

    geom::Rectangle view_area() const
    {
        return {geom::Point{}, display_device->display_size()};
    }

    void make_current()
    {
        if (eglMakeCurrent(egl_display, egl_surface,
                           egl_surface, egl_context) == EGL_FALSE)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("could not activate surface with eglMakeCurrent\n"));
        }
    }

    void release_current()
    {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE,
                       EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }

    void post_update()
    {
        display_device->commit_frame(egl_display, egl_surface);
    }

    bool can_bypass() const override
    {
        return false;
    }
    
protected:
    std::shared_ptr<ANativeWindow> const native_window;

    std::shared_ptr<mga::DisplayDevice> const display_device;
    EGLDisplay const egl_display;
    EGLConfig const egl_config;
    mg::EGLContextStore const egl_context;
    mg::EGLSurfaceStore const egl_surface;
    bool blanked;

};

}

std::unique_ptr<mg::DisplayBuffer> mga::DisplayBufferFactory::create_display_buffer(
    std::shared_ptr<ANativeWindow> const& native_window,
    std::shared_ptr<DisplayDevice> const& display_device,
    EGLDisplay egl_display, EGLConfig egl_config,
    EGLContext egl_context_shared)
{
    auto raw = new GPUDisplayBuffer(
        native_window, egl_display, egl_config, egl_context_shared, display_device);
    return std::unique_ptr<mg::DisplayBuffer>(raw);
}
