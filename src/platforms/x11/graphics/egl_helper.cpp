/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "egl_helper.h"

#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgx = mg::X;
namespace mgxh = mgx::helpers;

mgxh::EGLHelper::EGLHelper(GLConfig const& gl_config)
    : depth_buffer_bits{gl_config.depth_buffer_bits()},
      stencil_buffer_bits{gl_config.stencil_buffer_bits()},
      egl_display{EGL_NO_DISPLAY}, egl_config{0},
      egl_context{EGL_NO_CONTEXT}, egl_surface{EGL_NO_SURFACE},
      should_terminate_egl{false}
{
}

void mgxh::EGLHelper::setup(::Display* const x_dpy)
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    static const EGLint context_attr[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };

    setup_internal(x_dpy, true);

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

void mgxh::EGLHelper::setup(::Display* const x_dpy, EGLContext shared_context)
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    static const EGLint context_attr[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };

    setup_internal(x_dpy, false);

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

void mgxh::EGLHelper::setup(::Display* const x_dpy, Window win,
                            EGLContext shared_context)
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    static const EGLint context_attr[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
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
            eglBindAPI(MIR_SERVER_EGL_OPENGL_API);
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

bool mgxh::EGLHelper::swap_buffers()
{
    auto ret = eglSwapBuffers(egl_display, egl_surface);
    return (ret == EGL_TRUE);
}

bool mgxh::EGLHelper::make_current() const
{
    auto ret = eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);
    return (ret == EGL_TRUE);
}

bool mgxh::EGLHelper::release_current() const
{
    auto ret = eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    return (ret == EGL_TRUE);
}

void mgxh::EGLHelper::setup_internal(::Display* const x_dpy, bool initialize)
{
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, depth_buffer_bits,
        EGL_STENCIL_SIZE, stencil_buffer_bits,
        EGL_RENDERABLE_TYPE, MIR_SERVER_EGL_OPENGL_BIT,
        EGL_NONE
    };

    static const EGLint required_egl_version_major = 1;
    static const EGLint required_egl_version_minor = 4;

    EGLint num_egl_configs;

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

    if (eglChooseConfig(egl_display, config_attr, &egl_config, 1, &num_egl_configs) == EGL_FALSE ||
        num_egl_configs != 1)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to choose ARGB EGL config"));
    }
}

void mgxh::EGLHelper::report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)> f) const
{
    f(egl_display, egl_config);
}
