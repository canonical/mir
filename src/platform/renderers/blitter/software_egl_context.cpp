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

#define MIR_LOG_COMPONENT "BlitterRenderer"

#include "software_egl_context.h"

#include <mir/graphics/egl_error.h>
#include <mir/log.h>

#include <boost/throw_exception.hpp>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace mg = mir::graphics;
namespace mrb = mir::renderer::blitter;

namespace
{
/// Find an EGLDevice backed by a software rasteriser
///
/// \returns  The first device advertising EGL_MESA_device_software, or
///           EGL_NO_DEVICE_EXT if there is none.
auto find_software_device() -> EGLDeviceEXT
{
    // EGL_EXT_device_base is the union of EGL_EXT_device_enumeration and EGL_EXT_device_query
    auto const has_device_base = mg::has_egl_client_extension("EGL_EXT_device_base");
    if (!has_device_base &&
        (!mg::has_egl_client_extension("EGL_EXT_device_enumeration") ||
         !mg::has_egl_client_extension("EGL_EXT_device_query")))
    {
        return EGL_NO_DEVICE_EXT;
    }

    auto const egl_query_devices =
        reinterpret_cast<PFNEGLQUERYDEVICESEXTPROC>(eglGetProcAddress("eglQueryDevicesEXT")); //TICS !cppcoreguidelines-pro-type-reinterpret-cast: this is how EGL extension entrypoints are resolved
    auto const egl_query_device_string =
        reinterpret_cast<PFNEGLQUERYDEVICESTRINGEXTPROC>(eglGetProcAddress("eglQueryDeviceStringEXT")); //TICS !cppcoreguidelines-pro-type-reinterpret-cast: this is how EGL extension entrypoints are resolved
    if (!egl_query_devices || !egl_query_device_string)
    {
        return EGL_NO_DEVICE_EXT;
    }

    EGLint num_devices{0};
    if (egl_query_devices(0, nullptr, &num_devices) != EGL_TRUE || num_devices < 1)
    {
        return EGL_NO_DEVICE_EXT;
    }

    std::vector<EGLDeviceEXT> devices(num_devices);
    if (egl_query_devices(num_devices, devices.data(), &num_devices) != EGL_TRUE)
    {
        return EGL_NO_DEVICE_EXT;
    }
    devices.resize(num_devices);

    for (auto const device : devices)
    {
        auto const* const extensions = egl_query_device_string(device, EGL_EXTENSIONS);
        if (extensions && std::strstr(extensions, "EGL_MESA_device_software"))
        {
            return device;
        }
    }

    return EGL_NO_DEVICE_EXT;
}

auto create_software_display() -> EGLDisplay
{
    if (!mg::has_egl_client_extension("EGL_EXT_platform_base"))
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{
            "EGL implementation doesn't support EGL_EXT_platform_base; cannot select a software renderer"}));
    }

    mg::EGLExtensions::PlatformBaseEXT const platform_base;

    if (auto const device = find_software_device(); device != EGL_NO_DEVICE_EXT)
    {
        if (auto const dpy = platform_base.eglGetPlatformDisplay(EGL_PLATFORM_DEVICE_EXT, device, nullptr);
            dpy != EGL_NO_DISPLAY)
        {
            mir::log_debug("Blitter fallback rendering using EGL_MESA_device_software device");
            return dpy;
        }
    }

    // No explicitly-software device, so a surfaceless display is the next best thing.
    // It may well be hardware-backed, in which case rendering into the scanout
    // buffer's dma-buf may fail, which we will find out when we build the framebuffer pool.
    if (mg::has_egl_client_extension("EGL_MESA_platform_surfaceless"))
    {
        if (auto const dpy =
                platform_base.eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
            dpy != EGL_NO_DISPLAY)
        {
            mir::log_debug("Blitter fallback rendering using a surfaceless EGL display");
            return dpy;
        }
    }

    BOOST_THROW_EXCEPTION((std::runtime_error{"Failed to find an EGL device for blitter fallback rendering"}));
}

void require_display_extensions(EGLDisplay dpy)
{
    for (auto const* const extension :
         {"EGL_KHR_no_config_context", "EGL_KHR_surfaceless_context", "EGL_EXT_image_dma_buf_import"})
    {
        if (!mg::has_egl_extension(dpy, extension))
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{
                std::string{"EGL implementation missing necessary "} + extension + " extension"}));
        }
    }
}

auto initialise_display(EGLDisplay dpy) -> EGLDisplay
{
    EGLint major{0}, minor{0};
    if (eglInitialize(dpy, &major, &minor) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to initialise EGL display"));
    }

    try
    {
        require_display_extensions(dpy);
    }
    catch (...)
    {
        eglTerminate(dpy);
        throw;
    }

    return dpy;
}

auto create_context(EGLDisplay dpy) -> EGLContext
{
    static EGLint const context_attr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    eglBindAPI(EGL_OPENGL_ES_API);
    auto const ctx = eglCreateContext(dpy, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, context_attr);
    if (ctx == EGL_NO_CONTEXT)
    {
        // Capture the EGL error before eglTerminate() can overwrite it
        auto const error = mg::egl_error("Failed to create EGL context");
        eglTerminate(dpy);
        BOOST_THROW_EXCEPTION(error);
    }
    return ctx;
}
}

mrb::SoftwareEGLContext::SoftwareEGLContext()
    : dpy{initialise_display(create_software_display())},
      ctx{create_context(dpy)},
      exts{std::make_unique<mg::EGLExtensions>()}
{
    try
    {
        make_current();
    }
    catch (...)
    {
        eglDestroyContext(dpy, ctx);
        eglTerminate(dpy);
        throw;
    }
}

mrb::SoftwareEGLContext::~SoftwareEGLContext()
{
    eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroyContext(dpy, ctx);
    eglTerminate(dpy);
}

void mrb::SoftwareEGLContext::make_current() const
{
    if (eglGetCurrentContext() == ctx)
    {
        return;
    }
    eglBindAPI(EGL_OPENGL_ES_API);
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, ctx) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make blitter fallback EGL context current"));
    }
}

void mrb::SoftwareEGLContext::release_current() const
{
    if (eglGetCurrentContext() != ctx)
    {
        return;
    }
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release blitter fallback EGL context"));
    }
}
