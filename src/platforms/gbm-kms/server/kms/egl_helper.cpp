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

#include <epoxy/gl.h>
#include <epoxy/egl.h>

#include "egl_helper.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_error.h"
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#define MIR_LOG_COMPONENT "EGL"
#include "mir/log.h"

namespace mg = mir::graphics;
namespace mgg = mir::graphics::gbm;
namespace mgmh = mir::graphics::gbm::helpers;

mgmh::EGLHelper::EGLHelper(GLConfig const& gl_config, std::shared_ptr<EGLExtensions::DebugKHR> debug)
    : depth_buffer_bits{gl_config.depth_buffer_bits()},
      stencil_buffer_bits{gl_config.stencil_buffer_bits()},
      egl_display{EGL_NO_DISPLAY}, egl_config{0},
      egl_context{EGL_NO_CONTEXT}, egl_surface{EGL_NO_SURFACE},
      should_terminate_egl{false},
      debug{std::move(debug)}
{
}

mgmh::EGLHelper::EGLHelper(
    GLConfig const& gl_config,
    GBMHelper const& gbm,
    gbm_surface* surface,
    EGLContext shared_context,
    std::shared_ptr<EGLExtensions::DebugKHR> debug)
    : EGLHelper(gl_config, std::move(debug))
{
    setup(gbm, surface, shared_context, false);
}

mgmh::EGLHelper::EGLHelper(EGLHelper&& from)
    : depth_buffer_bits{from.depth_buffer_bits},
      stencil_buffer_bits{from.stencil_buffer_bits},
      egl_display{from.egl_display},
      egl_config{from.egl_config},
      egl_context{from.egl_context},
      egl_surface{from.egl_surface},
      should_terminate_egl{from.should_terminate_egl},
      debug{from.debug}
{
    from.should_terminate_egl = false;
    from.egl_display = EGL_NO_DISPLAY;
    from.egl_context = EGL_NO_CONTEXT;
    from.egl_surface = EGL_NO_SURFACE;
}

namespace
{
std::array<EGLint, 5> create_context_attr(EGLDisplay dpy, bool debug)
{
    std::array<EGLint, 5> context_array;
    context_array[0] = EGL_CONTEXT_CLIENT_VERSION;
    context_array[1] = 2;

    if (debug)
    {
        if (epoxy_egl_version(dpy) < 15)
        {
            if  (epoxy_has_egl_extension(dpy, "EGL_KHR_create_context"))
            {
                context_array[2] = EGL_CONTEXT_FLAGS_KHR;
                context_array[3] = EGL_CONTEXT_OPENGL_DEBUG_BIT_KHR;
            }
            else
            {
                mir::log_warning("Debug EGL context requested, but implementation does not support debug contexts");
            }
        }
        else
        {
            // EGL 1.5 includes EGL_CONTEXT_OPENGL_DEBUG attribute in core
            context_array[2] = EGL_CONTEXT_OPENGL_DEBUG;
            context_array[3] = EGL_TRUE;
        }
    }
    else
    {
        context_array[2] = EGL_NONE;
    }

    context_array[4] = EGL_NONE;
    return context_array;
}

void gl_logger(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei /*length*/,
    GLchar const* message,
    void const* /*userParam*/)
{
    auto const log_severity =
        [](GLenum severity)
        {
            switch(severity)
            {
            case GL_DEBUG_SEVERITY_HIGH:
                return mir::logging::Severity::error;
            case GL_DEBUG_SEVERITY_MEDIUM:
                return mir::logging::Severity::warning;
            case GL_DEBUG_SEVERITY_LOW:
                return mir::logging::Severity::informational;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                return mir::logging::Severity::debug;
            default:
                mir::log_error("Unknown GL severity %d. This is a Mir programming error.", severity);
                return mir::logging::Severity::error;
            }
        }(severity);
    auto const log_source =
        [](GLenum source)
        {
            switch(source)
            {
            case GL_DEBUG_SOURCE_API:
                return "GL";
            case GL_DEBUG_SOURCE_APPLICATION:
                return "Application";
            case GL_DEBUG_SOURCE_OTHER:
                return "Other";
            case GL_DEBUG_SOURCE_SHADER_COMPILER:
                return "Shader";
            case GL_DEBUG_SOURCE_THIRD_PARTY:
                return "Third Party";
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
                return "EGL";
            default:
                return "UNKNOWN";
            }
        }(source);

    auto const log_type =
        [](GLenum type)
        {
            switch(type)
            {
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                return "deprecated behaviour";
            case GL_DEBUG_TYPE_ERROR:
                return "error";
            case GL_DEBUG_TYPE_MARKER:
                return "marker";
            case GL_DEBUG_TYPE_OTHER:
                return "other";
            case GL_DEBUG_TYPE_PERFORMANCE:
                return "performance";
            case GL_DEBUG_TYPE_POP_GROUP:
                return "pop";
            case GL_DEBUG_TYPE_PORTABILITY:
                return "portability";
            case GL_DEBUG_TYPE_PUSH_GROUP:
                return "push";
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                return "undefined behaviour";
            default:
                return "UNKNOWN";
            }
        }(type);

    mir::log(
        log_severity,
        "GL",
        "%s: %s (id: 0x%X): %s",
        log_type,
        log_source,
        id,
        message);
}
}

void mgmh::EGLHelper::setup(GBMHelper const& gbm)
{
    eglBindAPI(EGL_OPENGL_ES_API);

    // TODO: Take the required format as a parameter, so we can select the framebuffer format.
    setup_internal(gbm, true, GBM_FORMAT_XRGB8888);

    auto const context_attr = create_context_attr(egl_display, static_cast<bool>(debug));

    egl_context = eglCreateContext(egl_display, egl_config, EGL_NO_CONTEXT, context_attr.data());
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));

    make_current();
    if (epoxy_has_gl_extension("GL_KHR_debug"))
    {
        glDebugMessageCallbackKHR(&gl_logger, nullptr);
        // Enable absolutely everything
        glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
    release_current();
}

void mgmh::EGLHelper::setup(GBMHelper const& gbm, EGLContext shared_context)
{
    eglBindAPI(EGL_OPENGL_ES_API);

    // TODO: Take the required format as a parameter, so we can select the framebuffer format.
    setup_internal(gbm, false, GBM_FORMAT_XRGB8888);

    auto const context_attr = create_context_attr(egl_display, static_cast<bool>(debug));

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr.data());
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));

    make_current();
    if (epoxy_has_gl_extension("GL_KHR_debug"))
    {
        glDebugMessageCallbackKHR(&gl_logger, nullptr);
        // Enable absolutely everything
        glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
    release_current();
}

void mgmh::EGLHelper::setup(
    GBMHelper const& gbm,
    gbm_surface* surface_gbm,
    EGLContext shared_context,
    bool owns_egl)
{
    eglBindAPI(EGL_OPENGL_ES_API);

    // TODO: Take the required format as a parameter, so we can select the framebuffer format.
    setup_internal(gbm, owns_egl, GBM_FORMAT_XRGB8888);

    egl_surface = platform_base.eglCreatePlatformWindowSurface(
        egl_display,
        egl_config,
        surface_gbm,
        nullptr);
    if(egl_surface == EGL_NO_SURFACE)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL window surface"));

    auto const context_attr = create_context_attr(egl_display, static_cast<bool>(debug));

    egl_context = eglCreateContext(egl_display, egl_config, shared_context, context_attr.data());
    if (egl_context == EGL_NO_CONTEXT)
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to create EGL context"));
    make_current();
    if (epoxy_has_gl_extension("GL_KHR_debug"))
    {
        glDebugMessageCallbackKHR(&gl_logger, nullptr);
        // Enable absolutely everything
        glDebugMessageControlKHR(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
    release_current();
}

mgmh::EGLHelper::~EGLHelper() noexcept
{
    if (egl_display != EGL_NO_DISPLAY)
    {
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

void mgmh::EGLHelper::set_debug_label(char const* label)
{
    auto const label_khr = static_cast<EGLLabelKHR>(const_cast<char*>(label));
    if (debug)
    {
        if (egl_display != EGL_NO_DISPLAY)
        {
            debug->eglLabelObjectKHR(
                egl_display,
                EGL_OBJECT_DISPLAY_KHR,
                egl_display,
                label_khr);
        }
        if (egl_context != EGL_NO_CONTEXT)
        {
            debug->eglLabelObjectKHR(
                egl_display,
                EGL_OBJECT_CONTEXT_KHR,
                egl_context,
                label_khr);
        }
        if (egl_surface != EGL_NO_SURFACE)
        {
            debug->eglLabelObjectKHR(
                egl_display,
                EGL_OBJECT_SURFACE_KHR,
                egl_surface,
                label_khr);
        }
    }
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

void mgmh::EGLHelper::setup_internal(GBMHelper const& gbm, bool initialize, EGLint gbm_format)
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

    static const EGLint required_egl_version_major = 1;
    static const EGLint required_egl_version_minor = 4;

    egl_display = platform_base.eglGetPlatformDisplay(
        EGL_PLATFORM_GBM_KHR,      // EGL_PLATFORM_GBM_MESA has the same value.
        static_cast<EGLNativeDisplayType>(gbm.device),
        nullptr);
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
