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

#include "mir/graphics/egl_extensions.h"
#define MIR_LOG_COMPONENT "gbm-kms"
#include "mir/log.h"

#include "rendering_platform.h"
#include "mir/module_deleter.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"
#include "mir/udev/wrapper.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/egl_error.h"
#include "mir/graphics/gl_config.h"
#include "mir/graphics/egl_logger.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>

namespace mg = mir::graphics;
namespace mo = mir::options;
namespace mge = mg::egl::generic;

auto create_rendering_platform(
    mg::SupportedDevice const&,
    std::vector<std::shared_ptr<mg::DisplayPlatform>> const& displays,
    mo::Option const&,
    mir::EmergencyCleanupRegistry&) -> mir::UniqueModulePtr<mg::RenderingPlatform>
{
   mir::assert_entry_point_signature<mg::CreateRenderPlatform>(&create_rendering_platform);

    return mir::make_module_ptr<mge::RenderingPlatform>(displays);
}

void add_graphics_platform_options(boost::program_options::options_description&)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

auto probe_rendering_platform(
    std::span<std::shared_ptr<mg::DisplayPlatform>> const& displays,
    mir::ConsoleServices&,
    std::shared_ptr<mir::udev::Context> const&,
    mo::ProgramOption const&) -> std::vector<mg::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::RenderProbe>(&probe_rendering_platform);

    mg::probe::Result maximum_suitability = mg::probe::unsupported;
    // First check if there are any displays we can possibly drive
    for (auto const& display_provider : displays)
    {
        if (display_provider->acquire_provider<mg::GenericEGLDisplayProvider>())
        {
            maximum_suitability = mg::probe::hosted;
            break;
        }
        if (display_provider->acquire_provider<mg::CPUAddressableDisplayProvider>())
        {
            // Check if the surfaceless platform is available
            if (mg::has_egl_client_extension("EGL_EXT_platform_base") &&
                mg::has_egl_client_extension("EGL_MESA_platform_surfaceless"))
            {
                // Check that we can actually initialise the EGL display
                mg::EGLExtensions ext;
                auto dpy=
                    ext.platform_base->eglGetPlatformDisplay(
                        EGL_PLATFORM_SURFACELESS_MESA,
                        EGL_DEFAULT_DISPLAY,
                        nullptr);
                EGLint major, minor;
                if (eglInitialize(dpy, &major, &minor) == EGL_TRUE)
                {
                    if (std::make_pair(major, minor) >= std::make_pair(1, 4))
                    {
                        // OK, EGL will somehow provide us with a usable display
                        maximum_suitability = mg::probe::supported;
                    }
                    eglTerminate(dpy);
                }
                continue;
            }
            // Check that EGL_DEFAULT_DISPLAY is something we can use...
            auto dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            EGLint major, minor;
            if (eglInitialize(dpy, &major, &minor) == EGL_TRUE)
            {
                if (std::make_pair(major, minor) >= std::make_pair(1, 4))
                {
                    // OK, EGL will somehow provide us with a usable display
                    maximum_suitability = mg::probe::supported;
                }
                eglTerminate(dpy);
            }
        }
    }

    std::vector<mg::SupportedDevice> supported_devices;
    supported_devices.push_back(
        mg::SupportedDevice {
            nullptr,                          // We aren't associated with any particular device
    
            maximum_suitability,              // We should be fully-functional, but let any hardware-specific
                                              // platform claim a higher priority, if it exists.
    
            nullptr                           // No platform-specific data
        });
    return supported_devices;
}

namespace
{
mir::ModuleProperties const description = {
    "mir:egl-generic",
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

