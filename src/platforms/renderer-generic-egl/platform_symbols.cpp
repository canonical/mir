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

#define MIR_LOG_COMPONENT "gbm-kms"
#include "mir/log.h"

#include "rendering_platform.h"
#include "mir/module_deleter.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"
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
    std::vector<std::shared_ptr<mg::DisplayPlatform>> const&,
    mo::Option const&,
    mir::EmergencyCleanupRegistry&) -> mir::UniqueModulePtr<mg::RenderingPlatform>
{
    mir::assert_entry_point_signature<mg::CreateRenderPlatform>(&create_rendering_platform);

    return mir::make_module_ptr<mge::RenderingPlatform>();
}

void add_graphics_platform_options(boost::program_options::options_description&)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
}

auto probe_rendering_platform(
    std::shared_ptr<mir::ConsoleServices> const&,
    std::shared_ptr<mir::udev::Context> const&,
    mo::ProgramOption const&) -> std::vector<mg::SupportedDevice>
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_rendering_platform);

    std::vector<mg::SupportedDevice> supported_devices;
    supported_devices.emplace_back(
        nullptr,                          // We aren't associated with any particular device
    
        mg::PlatformPriority::supported,  // We should be fully-functional, but let any hardware-specific
                                          // platform claim a higher priority, if it exists.
    
        nullptr);                         // No platform-specific data
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

