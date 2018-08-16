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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <epoxy/egl.h>

#include "platform.h"
#include "utils.h"
#include "mir/graphics/platform.h"
#include "mir/options/option.h"
#include "mir/module_deleter.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"
#include "mir/log.h"
#include "mir/graphics/egl_error.h"
#include "one_shot_device_observer.h"
#include "mir/raii.h"

#include <boost/throw_exception.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <xf86drm.h>
#include <sstream>

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <boost/exception/errinfo_file_name.hpp>

namespace mg = mir::graphics;
namespace mgc = mir::graphics::common;
namespace mo = mir::options;
namespace mge = mir::graphics::eglstream;

namespace
{
EGLDeviceEXT find_device()
{
    int device_count{0};
    if (eglQueryDevicesEXT(0, nullptr, &device_count) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to query device count with eglQueryDevicesEXT"));
    }

    auto devices = std::make_unique<EGLDeviceEXT[]>(device_count);
    if (eglQueryDevicesEXT(device_count, devices.get(), &device_count) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get device list with eglQueryDevicesEXT"));
    }

    auto device = std::find_if(devices.get(), devices.get() + device_count,
        [](EGLDeviceEXT device)
        {
            auto device_extensions = eglQueryDeviceStringEXT(device, EGL_EXTENSIONS);
            if (device_extensions)
            {
                return strstr(device_extensions, "EGL_EXT_device_drm") != NULL;
            }
            return false;
        });

    if (device == (devices.get() + device_count))
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Couldn't find EGLDeviceEXT supporting EGL_EXT_device_drm?"));
    }
    return *device;
}
}

mir::UniqueModulePtr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const&,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mg::DisplayReport> const&, 
    std::shared_ptr<mir::logging::Logger> const&)
{
    mir::assert_entry_point_signature<mg::CreateHostPlatform>(&create_host_platform);
    return mir::make_module_ptr<mge::Platform>(
        std::make_shared<mge::RenderingPlatform>(),
        std::make_shared<mge::DisplayPlatform>(*console, find_device()));
}

mir::UniqueModulePtr<mg::DisplayPlatform> create_display_platform(
    std::shared_ptr<mo::Option> const&, 
    std::shared_ptr<mir::EmergencyCleanupRegistry> const&,
    std::shared_ptr<mir::ConsoleServices> const& console,
    std::shared_ptr<mg::DisplayReport> const&, 
    std::shared_ptr<mir::logging::Logger> const&) 
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);
    return mir::make_module_ptr<mge::DisplayPlatform>(*console, find_device());
}

mir::UniqueModulePtr<mg::RenderingPlatform> create_rendering_platform(
    std::shared_ptr<mir::options::Option> const&,
    std::shared_ptr<mg::PlatformAuthentication> const&)
{
    mir::assert_entry_point_signature<mg::CreateRenderingPlatform>(&create_rendering_platform);
    return mir::make_module_ptr<mge::RenderingPlatform>();
}

void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

mg::PlatformPriority probe_graphics_platform(
    std::shared_ptr<mir::ConsoleServices> const& console,
    mo::ProgramOption const& /*options*/)
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_graphics_platform);

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
        return mg::PlatformPriority::unsupported;
    }

    int device_count{0};
    if (eglQueryDevicesEXT(0, nullptr, &device_count) != EGL_TRUE)
    {
        mir::log_info("Platform claims to support EGL_EXT_device_base, but "
                      "eglQueryDevicesEXT falied: %s",
                      mg::egl_category().message(eglGetError()).c_str());
        return mg::PlatformPriority::unsupported;
    }

    auto devices = std::make_unique<EGLDeviceEXT[]>(device_count);
    if (eglQueryDevicesEXT(device_count, devices.get(), &device_count) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to get device list with eglQueryDevicesEXT"));
    }

    if (std::none_of(devices.get(), devices.get() + device_count,
        [console](EGLDeviceEXT device)
        {
            auto device_extensions = eglQueryDeviceStringEXT(device, EGL_EXTENSIONS);
            if (device_extensions)
            {
                mir::log_debug("Found EGLDeviceEXT with device extensions: %s",
                               device_extensions);
                // TODO: This test is not strictly correct (will incorrectly match
                // EGL_EXT_device_drmish_but_not_drm)
                if (strstr(device_extensions, "EGL_EXT_device_drm") != NULL)
                {
                    // Check we can acquire the device...
                    mir::Fd drm_fd;
                    auto const devnum = mge::devnum_for_device(device);

                    auto device_holder = console->acquire_device(
                        major(devnum), minor(devnum),
                        std::make_unique<mgc::OneShotDeviceObserver>(drm_fd)).get();

                    if (drm_fd == mir::Fd::invalid)
                    {
                        mir::log_debug(
                            "EGL_EXT_device_drm found, but can't acquire DRM node.");
                        return false;
                    }

                    int const drm_node_attrib[] = {
                        EGL_DRM_MASTER_FD_EXT, static_cast<int>(drm_fd), EGL_NONE
                    };
                    EGLDisplay display = eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, device, drm_node_attrib);

                    if (display == EGL_NO_DISPLAY)
                    {
                        mir::log_debug("Failed to create EGLDisplay: %s", mg::egl_category().message(eglGetError()).c_str());
                        return false;
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

                    if (display == EGL_NO_DISPLAY)
                    {
                        return false;
                    }

                    if (epoxy_has_egl_extension(display, "EGL_EXT_output_base"))
                    {
                        return true;
                    }
                    else
                    {
                        mir::log_debug("EGL_EXT_device_drm found, but missing EGL_EXT_output_base extension.");
                        mir::log_debug("Available extensions are: %s", eglQueryString(display, EGL_EXTENSIONS));
                        return false;
                    }
                }
                return false;
            }
            else
            {
                mir::log_debug("Found EGLDeviceEXT with no device extensions");
                return false;
            }
        }))
    {
        mir::log_debug(
            "EGLDeviceEXTs found, but none are suitable for Mir");
        return mg::PlatformPriority::unsupported;
    }

    return mg::PlatformPriority::best;
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

