/*
 * Copyright © 2017 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "egl_helper.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#define MIR_LOG_COMPONENT "EGL"
#include "mir/log.h"

namespace mg = mir::graphics;
namespace mgm = mir::graphics::mesa;
namespace mgmh = mir::graphics::mesa::helpers;

mgmh::EGLHelper::EGLHelper(GLConfig const& gl_config)
    : depth_buffer_bits{gl_config.depth_buffer_bits()},
      stencil_buffer_bits{gl_config.stencil_buffer_bits()},
      egl_display{EGL_NO_DISPLAY}, egl_config{0},
      egl_context{EGL_NO_CONTEXT}, egl_surface{EGL_NO_SURFACE},
      should_terminate_egl{false}
{
}

mgmh::EGLHelper::EGLHelper(
    GLConfig const& gl_config,
    GBMHelper const& gbm,
    gbm_surface* surface,
    EGLContext shared_context)
    : EGLHelper(gl_config)
{
    setup(gbm, surface, shared_context);
}

mgmh::EGLHelper::EGLHelper(EGLHelper&& from)
    : depth_buffer_bits{from.depth_buffer_bits},
      stencil_buffer_bits{from.stencil_buffer_bits},
      egl_display{from.egl_display},
      egl_config{from.egl_config},
      egl_context{from.egl_context},
      egl_surface{from.egl_surface},
      should_terminate_egl{from.should_terminate_egl}
{
    from.should_terminate_egl = false;
    from.egl_display = EGL_NO_DISPLAY;
    from.egl_context = EGL_NO_CONTEXT;
    from.egl_surface = EGL_NO_SURFACE;
}

void mgmh::EGLHelper::setup(GBMHelper const& gbm)
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    static const EGLint context_attr[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };

    // TODO: Take the required format as a parameter, so we can select the framebuffer format.
    setup_internal(gbm, true, GBM_FORMAT_XRGB8888);

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

void mgmh::EGLHelper::setup(GBMHelper const& gbm, EGLContext shared_context)
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    static const EGLint context_attr[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };

    // TODO: Take the required format as a parameter, so we can select the framebuffer format.
    setup_internal(gbm, false, GBM_FORMAT_XRGB8888);

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

void mgmh::EGLHelper::setup(GBMHelper const& gbm, gbm_surface* surface_gbm,
                            EGLContext shared_context)
{
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);

    static const EGLint context_attr[] = {
#if MIR_SERVER_EGL_OPENGL_BIT == EGL_OPENGL_ES2_BIT
        EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
        EGL_NONE
    };

    // TODO: Take the required format as a parameter, so we can select the framebuffer format.
    setup_internal(gbm, false, GBM_FORMAT_XRGB8888);

    egl_surface = eglCreateWindowSurface(egl_display, egl_config, surface_gbm, nullptr);
    if(egl_surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL window surface"));

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

mgmh::EGLHelper::~EGLHelper() noexcept
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

bool mgmh::EGLHelper::swap_buffers()
{
    auto ret = eglSwapBuffers(egl_display, egl_surface);
    return (ret == EGL_TRUE);
}

bool mgmh::EGLHelper::make_current() const
{
    auto ret = eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    eglBindAPI(MIR_SERVER_EGL_OPENGL_API);
    return (ret == EGL_TRUE);
}

bool mgmh::EGLHelper::release_current() const
{
    auto ret = eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    return (ret == EGL_TRUE);
}

namespace
{
std::vector<EGLConfig> get_matching_configs(EGLDisplay dpy, EGLint const attr[])
{
    EGLint num_egl_configs;

    // First query the number of matching configs…
    if ((eglChooseConfig(dpy, attr, nullptr, 0, &num_egl_configs) == EGL_FALSE) ||
        (num_egl_configs == 0))
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to choose EGL config"));
    }

    std::vector<EGLConfig> matching_configs(static_cast<size_t>(num_egl_configs));
    if ((eglChooseConfig(dpy, attr, matching_configs.data(), static_cast<EGLint>(matching_configs.size()), &num_egl_configs) == EGL_FALSE) ||
        (num_egl_configs == 0))
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to choose EGL config"));
    }

    matching_configs.resize(static_cast<size_t>(num_egl_configs));
    return matching_configs;
}
}

void mgmh::EGLHelper::setup_internal(GBMHelper const& gbm, bool initialize, EGLint gbm_format)
{
    // TODO: Get the required EGL_{RED,GREEN,BLUE}_SIZE values out of gbm_format
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 5,
        EGL_GREEN_SIZE, 5,
        EGL_BLUE_SIZE, 5,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, depth_buffer_bits,
        EGL_STENCIL_SIZE, stencil_buffer_bits,
        EGL_RENDERABLE_TYPE, MIR_SERVER_EGL_OPENGL_BIT,
        EGL_NONE
    };

    static const EGLint required_egl_version_major = 1;
    static const EGLint required_egl_version_minor = 4;

    egl_display = eglGetDisplay(static_cast<EGLNativeDisplayType>(gbm.device));
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

    for (auto const& config : get_matching_configs(egl_display, config_attr))
    {
        EGLint id;
        if (eglGetConfigAttrib(egl_display, config, EGL_NATIVE_VISUAL_ID, &id) == EGL_FALSE)
        {
            mir::log_warning(
                "Failed to query GBM format of EGLConfig: %s",
                mg::egl_category().message(eglGetError()).c_str());
            continue;
        }

        if (id == gbm_format)
        {
            // We've found our matching format, so we're done here.
            egl_config = config;
            return;
        }
    }
    BOOST_THROW_EXCEPTION((
        std::runtime_error{std::string{"Failed to find EGL config matching "} + std::to_string(gbm_format)}));
}

void mgmh::EGLHelper::report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)> f)
{
    f(egl_display, egl_config);
}
