/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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

#include "platform.h"

namespace mg = mir::graphics;
namespace mgx = mg::X;
namespace mo = mir::options;

mgx::Platform::Platform(std::shared_ptr<DisplayReport> const& /*listener*/)
{
}

std::shared_ptr<mg::GraphicBufferAllocator> mgx::Platform::create_buffer_allocator()
{
//    return std::make_shared<mgm::BufferAllocator>(gbm.device, bypass_option_);
    return nullptr;
}

std::shared_ptr<mg::Display> mgx::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
    std::shared_ptr<GLProgramFactory> const&,
    std::shared_ptr<GLConfig> const& /*gl_config*/)
{
#if 0
    return std::make_shared<mgm::Display>(
        this->shared_from_this(),
        initial_conf_policy,
        gl_config,
        listener);
#else
    return nullptr;
#endif
}

std::shared_ptr<mg::PlatformIpcOperations> mgx::Platform::make_ipc_operations() const
{
//    return std::make_shared<mgm::IpcOperations>(drm);
    return nullptr;
}

EGLNativeDisplayType mgx::Platform::egl_native_display() const
{
//    return gbm.device;
	return 0;
}

extern "C" std::shared_ptr<mg::Platform> mg::create_host_platform(
    std::shared_ptr<mo::Option> const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<DisplayReport> const& report)
{
    return std::make_shared<mgx::Platform>(report);
}

extern "C" std::shared_ptr<mg::Platform> create_guest_platform(
    std::shared_ptr<mg::DisplayReport> const& report,
    std::shared_ptr<mg::NestedContext> const&)
{
    return std::make_shared<mgx::Platform>(report);
}

extern "C" void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
#if 0
    config.add_options()
        (vt_option_name,
         boost::program_options::value<int>()->default_value(0),
         "[platform-specific] VT to run on or 0 to use current.")
        (bypass_option_name,
         boost::program_options::value<bool>()->default_value(true),
         "[platform-specific] utilize the bypass optimization for fullscreen surfaces.");
#endif
}

extern "C" mg::PlatformPriority probe_graphics_platform()
{
#if 0
    auto udev = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator drm_devices{udev};
    drm_devices.match_subsystem("drm");
    drm_devices.match_sysname("card[0-9]*");
    drm_devices.scan_devices();

    for (auto& device : drm_devices)
    {
        static_cast<void>(device);
        return mg::PlatformPriority::best;
    }
#endif
    return mg::PlatformPriority::best;
}

mir::ModuleProperties const description = {
    "X",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO
};

extern "C" mir::ModuleProperties const* describe_graphics_module()
{
    return &description;
}
