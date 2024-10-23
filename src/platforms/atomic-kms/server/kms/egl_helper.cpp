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
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>
#include <gbm.h>

#include "mir/log.h"

namespace mg = mir::graphics;
namespace mgmh = mir::graphics::atomic::helpers;

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
    uint32_t gbm_format,
    EGLContext shared_context)
    : EGLHelper(gl_config)
{
    setup(gbm.device, surface, gbm_format, shared_context, false);
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

namespace
{
void initialise_egl(EGLDisplay dpy, int minimum_major_version, int minimum_minor_version)
{
    EGLint major, minor;

    if (eglInitialize(dpy, &major, &minor) == EGL_FALSE)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialize EGL display"));

    if ((major < minimum_major_version) ||
        (major == minimum_major_version && minor < minimum_minor_version))
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(std::runtime_error("Incompatible EGL version")));
        // TODO: Insert egl version major and minor into exception
    }
}

std::vector<EGLConfig> get_matching_configs(EGLDisplay dpy, EGLint const attr[])
{
    EGLint num_egl_configs;

    // First query the number of matching configs…
    if ((eglChooseConfig(dpy, attr, nullptr, 0, &num_egl_configs) == EGL_FALSE) ||
        (num_egl_configs == 0))
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to enumerate any matching EGL configs"));
    }

    std::vector<EGLConfig> matching_configs(static_cast<size_t>(num_egl_configs));
    if ((eglChooseConfig(dpy, attr, matching_configs.data(), static_cast<EGLint>(matching_configs.size()), &num_egl_configs) == EGL_FALSE) ||
        (num_egl_configs == 0))
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to acquire matching EGL configs"));
    }

    matching_configs.resize(static_cast<size_t>(num_egl_configs));
    return matching_configs;
}

}

void mgmh::EGLHelper::setup(GBMHelper const& gbm)
{
    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    egl_display = egl_display_for_gbm_device(gbm.device);
    initialise_egl(egl_display, 1, 4);
    should_terminate_egl = true;

    // This is a context solely used for sharing GL object IDs; we will not do any rendering
    // with it, so we do not care what EGLconfig is used *at all*.
    EGLint const no_attribs[] = {EGL_NONE};
    egl_config = get_matching_configs(egl_display, no_attribs)[0];

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

void mgmh::EGLHelper::setup(GBMHelper const& gbm, EGLContext shared_context)
{
    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    egl_display = egl_display_for_gbm_device(gbm.device);

    // Might as well copy the EGLConfig from shared_context
    EGLint config_id;
    if (eglQueryContext(egl_display, shared_context, EGL_CONFIG_ID, &config_id) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query EGLConfig of shared EGLContext"));
    }
    EGLint const context_attribs[] = {
        EGL_CONFIG_ID, config_id,
        EGL_NONE
    };
    egl_config = get_matching_configs(egl_display, context_attribs)[0];

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr);
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
}

void mgmh::EGLHelper::setup(
    gbm_device* const device,
    gbm_surface* surface_gbm,
    uint32_t gbm_format,
    EGLContext shared_context,
    bool owns_egl)
{
    eglBindAPI(EGL_OPENGL_ES_API);

    static const EGLint context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    egl_display = egl_display_for_gbm_device(device);
    if (owns_egl)
    {
        initialise_egl(egl_display, 1, 4);
        should_terminate_egl = owns_egl;
    }

    egl_config = egl_config_for_format(static_cast<EGLint>(gbm_format));

    egl_surface = platform_base.eglCreatePlatformWindowSurface(
        egl_display,
        egl_config,
        surface_gbm,
        nullptr);
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

bool mgmh::EGLHelper::swap_buffers()
{
    auto ret = eglSwapBuffers(egl_display, egl_surface);
    return (ret == EGL_TRUE);
}

bool mgmh::EGLHelper::make_current() const
{
    auto ret = eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);
    eglBindAPI(EGL_OPENGL_ES_API);
    return (ret == EGL_TRUE);
}

bool mgmh::EGLHelper::release_current() const
{
    auto ret = eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    return (ret == EGL_TRUE);
}

auto mgmh::EGLHelper::egl_display_for_gbm_device(struct gbm_device* const device) -> EGLDisplay
{
    auto const egl_display = platform_base.eglGetPlatformDisplay(
        EGL_PLATFORM_GBM_KHR,      // EGL_PLATFORM_GBM_MESA has the same value.
        static_cast<EGLNativeDisplayType>(device),
        nullptr);
    if (egl_display == EGL_NO_DISPLAY)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get EGL display"));

    return egl_display;
}

auto mgmh::EGLHelper::egl_config_for_format(EGLint gbm_format) -> EGLConfig
{
    // TODO: Get the required EGL_{RED,GREEN,BLUE}_SIZE values out of gbm_format
    EGLint const config_attr[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_DEPTH_SIZE, depth_buffer_bits,
        EGL_STENCIL_SIZE, stencil_buffer_bits,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

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
            return config;
        }
    }
    BOOST_THROW_EXCEPTION((NoMatchingEGLConfig{static_cast<uint32_t>(gbm_format)}));
}

void mgmh::EGLHelper::report_egl_configuration(std::function<void(EGLDisplay, EGLConfig)> f)
{
    f(egl_display, egl_config);
}

mgmh::EGLHelper::NoMatchingEGLConfig::NoMatchingEGLConfig(uint32_t /*format*/)
    : std::runtime_error("Failed to find matching EGL config")
{
    // TODO: Include the format string; need to extract from linux_dmabuf.cpp
}
