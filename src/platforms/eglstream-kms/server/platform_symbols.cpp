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

#include <epoxy/egl.h>

#include "platform.h"
#include "utils.h"
#include "mir/graphics/platform.h"
#include "mir/options/option.h"
#include "mir/options/configuration.h"
#include "mir/module_deleter.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"
#include "mir/log.h"
#include "mir/graphics/egl_error.h"
#include "one_shot_device_observer.h"
#include "mir/raii.h"
#include "kms-utils/drm_mode_resources.h"
#include "mir/graphics/egl_logger.h"
#include "mir/udev/wrapper.h"

#include <boost/throw_exception.hpp>
#include <xf86drm.h>
#include <sstream>

#include <fcntl.h>
#include <sys/sysmacros.h>
#include <boost/exception/errinfo_file_name.hpp>
#include <xf86drmMode.h>

namespace mg = mir::graphics;
namespace mo = mir::options;
namespace mge = mir::graphics::eglstream;

auto create_rendering_platform(
    mg::SupportedDevice const& device,
    std::vector<std::shared_ptr<mg::DisplayInterfaceProvider>> const& /*displays*/,
    mo::Option const&,
    mir::EmergencyCleanupRegistry&) -> mir::UniqueModulePtr<mg::RenderingPlatform>
{
    mir::assert_entry_point_signature<mg::CreateRenderPlatform>(&create_rendering_platform);


    EGLDisplay display{EGL_NO_DISPLAY};
    display = eglGetPlatformDisplayEXT(
        EGL_PLATFORM_DEVICE_EXT,
        std::any_cast<EGLDeviceEXT>(device.platform_data),
        nullptr);

    if (display == EGL_NO_DISPLAY)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to create EGL Display")));
    }

    EGLint major_ver{1}, minor_ver{4};
    if (!eglInitialize(display, &major_ver, &minor_ver))
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to initialise EGL")));
    }

    return mir::make_module_ptr<mge::RenderingPlatform>(display);
}

void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

auto probe_rendering_platform(
    std::shared_ptr<mir::ConsoleServices> const& /*console*/,
    std::shared_ptr<mir::udev::Context> const& udev,
    mo::ProgramOption const& /*options*/) -> std::vector<mg::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_rendering_platform);

    std::vector<char const*> missing_extensions;
    for (char const* extension : {
        "EGL_EXT_platform_base",
        "EGL_EXT_platform_device",
        "EGL_EXT_device_base",})
    {
        if (!epoxy_has_egl_extension(EGL_NO_DISPLAY, extension))
        {
            missing_extensions.push_back(extension);
        }
    }

    if (!missing_extensions.empty())
    {
        std::stringstream message;
        message << "Missing required extension" << (missing_extensions.size() > 1 ? "s:" : ":");
        for (auto missing_extension : missing_extensions)
        {
            message << " " << missing_extension;
        }

        mir::log_debug("EGLStream platform is unsupported: %s",
                       message.str().c_str());
        return {};
    }

    int device_count{0};
    if (eglQueryDevicesEXT(0, nullptr, &device_count) != EGL_TRUE)
    {
        mir::log_info("Platform claims to support EGL_EXT_device_base, but "
                      "eglQueryDevicesEXT falied: %s",
                      mg::egl_category().message(eglGetError()).c_str());
        return {};
    }

    auto devices = std::make_unique<EGLDeviceEXT[]>(device_count);
    if (eglQueryDevicesEXT(device_count, devices.get(), &device_count) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get device list with eglQueryDevicesEXT"));
    }

    std::vector<mg::SupportedDevice> supported_devices;
    for (auto i = 0; i != device_count; ++i)
    {
        auto const& device = devices[i];
        try
        {
            supported_devices.emplace_back(mg::SupportedDevice{
                udev->char_device_from_devnum(mge::devnum_for_device(device)),
                mg::PlatformPriority::unsupported,
                device
            });
        }
        catch (std::exception const& e)
        {
            mir::log_debug("Failed to find kernel device for EGLDevice: %s", e.what());
            continue;
        }

        EGLDisplay display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, device, nullptr);

        if (display == EGL_NO_DISPLAY)
        {
            mir::log_debug("Failed to create EGLDisplay: %s", mg::egl_category().message(eglGetError()).c_str());
            continue;
        }

        auto egl_init = mir::raii::paired_calls(
            [&display]()
            {
                EGLint major_ver{1}, minor_ver{4};
                if (!eglInitialize(display, &major_ver, &minor_ver))
                {
                    mir::log_debug("Failed to initialise EGL: %s", mg::egl_category().message(eglGetError()).c_str());
                    display = EGL_NO_DISPLAY;
                }
            },
            [&display]()
            {
                if (display != EGL_NO_DISPLAY)
                {
                    eglTerminate(display);
                }
            });

        if (display != EGL_NO_DISPLAY)
        {
            std::vector<char const*> missing_extensions;
            for (char const* extension : {
                "EGL_KHR_stream_consumer_gltexture",
                "EGL_NV_stream_attrib"})
            {
                if (!epoxy_has_egl_extension(display, extension))
                {
                    missing_extensions.push_back(extension);
                }
            }

            for (auto const missing_extension: missing_extensions)
            {
                mir::log_info("EGLDevice found but unsuitable. Missing extension %s", missing_extension);
            }

            if (missing_extensions.empty())
            {
                // We've got EGL, and we've got the necessary EGL extensions. We're good.
                supported_devices.back().support_level = mg::PlatformPriority::best;
            }
        }
    }
    if (!std::any_of(
        supported_devices.begin(),
        supported_devices.end(),
        [](auto const& device)
        {
            return device.support_level > mg::PlatformPriority::unsupported;
        }))
    {
        mir::log_debug(
            "EGLDeviceEXTs found, but none are suitable for Mir");
    }

    return supported_devices;
}

namespace
{
mir::ModuleProperties const description = {
    "mir:eglstream-kms",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO,
    mir::libname()
};
}

mir::ModuleProperties const* describe_graphics_module()
{
    mir::assert_entry_point_signature<mg::DescribeModule>(&describe_graphics_module);
    return &description;
}

