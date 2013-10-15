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

#include "gpu_android_display_buffer_factory.h"
#include "hwc_android_display_buffer_factory.h"
#include "android_framebuffer_window_query.h"
#include "hwc_device.h"

#include "mir/graphics/display_buffer.h"
#include "mir/graphics/egl_resources.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mg = mir::graphics;
namespace mga = mir::graphics::android;
namespace geom = mir::geometry;

namespace
{

EGLContext create_context(mga::AndroidFramebufferWindowQuery& native_window,
                          EGLDisplay egl_display,
                          EGLContext egl_context_shared)
{
    static EGLint const default_egl_context_attr[] =
    {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    auto egl_config = native_window.android_display_egl_config(egl_display);
    return eglCreateContext(egl_display, egl_config, egl_context_shared,
                            default_egl_context_attr);
}

EGLSurface create_surface(mga::AndroidFramebufferWindowQuery& native_window,
                          EGLDisplay egl_display)
{
    auto egl_config = native_window.android_display_egl_config(egl_display);
    auto native_win_type = native_window.android_native_window_type();

    return eglCreateWindowSurface(egl_display, egl_config, native_win_type, NULL);
}

class GPUDisplayBuffer : public mg::DisplayBuffer
{
public:
    GPUDisplayBuffer(std::shared_ptr<mga::AndroidFramebufferWindowQuery> const& native_window,
                     EGLDisplay egl_display,
                     EGLContext egl_context_shared)
        : native_window{native_window},
          egl_display{egl_display},
          egl_context{egl_display, create_context(*native_window, egl_display, egl_context_shared)},
          egl_surface{egl_display, create_surface(*native_window, egl_display)}
    {

    }

    geom::Rectangle view_area() const
    {
        int display_width, display_height;
        eglQuerySurface(egl_display, egl_surface, EGL_WIDTH, &display_width);
        eglQuerySurface(egl_display, egl_surface, EGL_HEIGHT, &display_height);
        geom::Width w{display_width};
        geom::Height h{display_height};

        return {geom::Point{}, geom::Size{w,h}};
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
        if (eglSwapBuffers(egl_display, egl_surface) == EGL_FALSE)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error("eglSwapBuffers failure\n"));
        }
    }

    bool can_bypass() const override
    {
        return false;
    }
    
protected:
    std::shared_ptr<mga::AndroidFramebufferWindowQuery> const native_window;
    EGLDisplay const egl_display;
    mg::EGLContextStore const egl_context;
    mg::EGLSurfaceStore const egl_surface;
};

class HWCDisplayBuffer : public GPUDisplayBuffer
{
public:
    HWCDisplayBuffer(std::shared_ptr<mga::AndroidFramebufferWindowQuery> const& native_win,
                     EGLDisplay egl_display,
                     EGLContext egl_context_shared,
                     std::shared_ptr<mga::HWCDevice> const& hwc_device)
        : GPUDisplayBuffer{native_win, egl_display, egl_context_shared},
          hwc_device{hwc_device},
          blanked(false)
    {
    }

    geom::Rectangle view_area() const override
    {
        geom::Point origin_pt{0, 0};
        auto size = hwc_device->display_size();
        return geom::Rectangle{origin_pt, size};
    }

    void post_update() override
    {
        hwc_device->commit_frame(egl_display, egl_surface);
    }

private:
    std::shared_ptr<mga::HWCDevice> const hwc_device;
    
    bool blanked;
};

}

std::unique_ptr<mg::DisplayBuffer> mga::GPUAndroidDisplayBufferFactory::create_display_buffer(
    std::shared_ptr<AndroidFramebufferWindowQuery> const& native_window,
    EGLDisplay egl_display,
    EGLContext egl_context_shared)
{
    auto raw = new GPUDisplayBuffer(native_window, egl_display, egl_context_shared);
    return std::unique_ptr<mg::DisplayBuffer>(raw);
}

mga::HWCAndroidDisplayBufferFactory::HWCAndroidDisplayBufferFactory(
    std::shared_ptr<mga::HWCDevice> const& hwc_device)
    : hwc_device{hwc_device}
{
}

std::unique_ptr<mg::DisplayBuffer> mga::HWCAndroidDisplayBufferFactory::create_display_buffer(
    std::shared_ptr<AndroidFramebufferWindowQuery> const& native_window,
    EGLDisplay egl_display,
    EGLContext egl_context_shared)
{
    auto raw = new HWCDisplayBuffer(native_window, egl_display, egl_context_shared, hwc_device);
    return std::unique_ptr<mg::DisplayBuffer>(raw);
}
