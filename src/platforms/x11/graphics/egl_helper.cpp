/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "egl_helper.h"

#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"

#include <EGL/egl.h>
#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgx = mg::X;
namespace mgxh = mgx::helpers;

class mgxh::Framebuffer::EGLState
{
public:
    EGLState(EGLDisplay dpy, EGLContext ctx, EGLSurface surf)
        : dpy{dpy},
          ctx{ctx},
          surf{surf}
    {
    }

    ~EGLState()
    {
        eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(dpy, ctx);
        eglDestroySurface(dpy, surf);
    }

    auto format() const -> MirPixelFormat
    {
        EGLint texture_format = -1;
        eglQuerySurface(dpy, surf, EGL_TEXTURE_FORMAT, &texture_format);
        
        switch(texture_format)
        {
            case EGL_TEXTURE_RGB:
                return mir_pixel_format_rgb_888;
            case EGL_TEXTURE_RGBA:
                return mir_pixel_format_argb_8888;
            default:
                return mir_pixel_format_invalid;
        }
    }
    
    EGLDisplay const dpy;
    EGLContext const ctx;
    EGLSurface const surf;
};

mgxh::Framebuffer::Framebuffer(EGLDisplay dpy, EGLContext ctx, EGLSurface surf, geometry::Size size)
    : Framebuffer(std::make_shared<EGLState>(dpy, ctx, surf), size)
{
}

mgxh::Framebuffer::Framebuffer(std::shared_ptr<EGLState const> state, geometry::Size size)
    : state{std::move(state)},
      size_{size}
{
}

auto mgxh::Framebuffer::size() const -> geometry::Size
{
    return size_;
}

auto mgxh::Framebuffer::format() const -> MirPixelFormat
{
    return state->format();
}

void mgxh::Framebuffer::make_current()
{
    if (eglMakeCurrent(state->dpy, state->surf, state->surf, state->ctx) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("eglMakeCurrent failed")));
    }
}

void mgxh::Framebuffer::release_current()
{
    if (eglMakeCurrent(state->dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release EGL context"));
    }
}

void mgxh::Framebuffer::swap_buffers()
{
    if (eglSwapBuffers(state->dpy, state->surf) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("eglSwapBuffers failed")));
    }
}

auto mgxh::Framebuffer::clone_handle() -> std::unique_ptr<mg::GenericEGLDisplayAllocator::EGLFramebuffer>
{
    return std::unique_ptr<mg::GenericEGLDisplayAllocator::EGLFramebuffer>{new Framebuffer(state, size_)};
}

mgxh::EGLHelper::EGLHelper(::Display* const x_dpy)
    : EGLHelper{0, 0}
{   
    setup_internal(x_dpy, true);
}

mgxh::EGLHelper::EGLHelper(GLConfig const& gl_config, ::Display* const x_dpy)
    : EGLHelper{gl_config.stencil_buffer_bits(), gl_config.depth_buffer_bits()}
{
    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    setup_internal(x_dpy, true);

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

mgxh::EGLHelper::EGLHelper(
    GLConfig const& gl_config,
    ::Display* const x_dpy,
    xcb_window_t win,
    EGLContext shared_context)
    : EGLHelper{gl_config.stencil_buffer_bits(), gl_config.depth_buffer_bits()}
{
    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    setup_internal(x_dpy, false);

    egl_surface = eglCreateWindowSurface(egl_display, egl_config, win, nullptr);
    if(egl_surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL window surface"));

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

mgxh::EGLHelper::~EGLHelper() noexcept
{
    if (egl_display != EGL_NO_DISPLAY) {
        if (egl_context != EGL_NO_CONTEXT)
        {
            eglBindAPI(EGL_OPENGL_ES_API);
            if (eglGetCurrentContext() == egl_context)
                eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            eglDestroyContext(egl_display, egl_context);
        }
        if (egl_surface != EGL_NO_SURFACE)
            eglDestroySurface(egl_display, egl_surface);
        if (should_terminate_egl)
            eglTerminate(egl_display);
    }
}

mgxh::EGLHelper::EGLHelper(int stencil_bits, int depth_bits)
    : depth_buffer_bits{depth_bits},
      stencil_buffer_bits{stencil_bits},
      egl_display{EGL_NO_DISPLAY}, egl_config{0},
      egl_context{EGL_NO_CONTEXT}, egl_surface{EGL_NO_SURFACE},
      should_terminate_egl{false}
{
}

namespace
{
auto size_for_x_win(xcb_connection_t* xcb_conn, xcb_window_t win) -> mir::geometry::Size
{
    auto cookie = xcb_get_geometry(xcb_conn, win);
    if (auto reply = xcb_get_geometry_reply(xcb_conn, cookie, nullptr))
    {
        mir::geometry::Size const window_size{reply->width, reply->height};
        free(reply);
        return window_size;
    }
    BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to get X11 window size"}));
}
}

auto mgxh::EGLHelper::framebuffer_for_window(
    GLConfig const& conf,
    xcb_connection_t* xcb_conn,
    xcb_window_t win,
    EGLContext shared_context) -> std::unique_ptr<Framebuffer>
{
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, conf.depth_buffer_bits(),
        EGL_STENCIL_SIZE, conf.stencil_buffer_bits(),
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint num_egl_configs;
    if (eglChooseConfig(egl_display, config_attr, &egl_config, 1, &num_egl_configs) == EGL_FALSE ||
        num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to choose ARGB EGL config"));
    }

    auto egl_surface = eglCreateWindowSurface(egl_display, egl_config, win, nullptr);
    if(egl_surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL window surface"));

    auto egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));

    return std::make_unique<Framebuffer>(
        egl_display,
        egl_context,
        egl_surface,
        size_for_x_win(xcb_conn, win));
}

void mgxh::EGLHelper::setup_internal(::Display* x_dpy, bool initialize)
{

    static const EGLint required_egl_version_major = 1;
    static const EGLint required_egl_version_minor = 4;

    egl_display = eglGetDisplay(static_cast<EGLNativeDisplayType>(x_dpy));
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get EGL display"));

    if (initialize)
    {
        EGLint major, minor;

        if (eglInitialize(egl_display, &major, &minor) == EGL_FALSE)
            BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialize EGL display"));

        if ((major < required_egl_version_major) ||
            (major == required_egl_version_major && minor < required_egl_version_minor))
        {
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(std::runtime_error("Incompatible EGL version")));
            // TODO: Insert egl version major and minor into exception
        }

        should_terminate_egl = true;
    }
}

void mgxh::EGLHelper::report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)> f) const
{
    f(egl_display, egl_config);
}
