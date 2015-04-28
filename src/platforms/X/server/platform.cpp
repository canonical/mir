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
#include "display.h"
#include "buffer_allocator.h"
#include "ipc_operations.h"
#include "mir/udev/wrapper.h"
#include "../debug.h"

namespace mg = mir::graphics;
namespace mgx = mg::X;
namespace mo = mir::options;

mgx::Platform::Platform(std::shared_ptr<DisplayReport> const& /*listener*/)
    : udev{std::make_shared<mir::udev::Context>()},
       drm{std::make_shared<helpers::DRMHelper>()}
{
   CALLED

   drm->setup(udev);
   gbm.setup(*drm);
}

std::shared_ptr<mg::GraphicBufferAllocator> mgx::Platform::create_buffer_allocator()
{
    CALLED
    return std::make_shared<mgx::BufferAllocator>(gbm.device);
}

std::shared_ptr<mg::Display> mgx::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
    std::shared_ptr<GLProgramFactory> const&,
    std::shared_ptr<GLConfig> const& /*gl_config*/)
{
    CALLED
    return std::make_shared<mgx::Display>();
}

std::shared_ptr<mg::PlatformIpcOperations> mgx::Platform::make_ipc_operations() const
{
    CALLED
    return std::make_shared<mgx::IpcOperations>();
}

EGLNativeDisplayType mgx::Platform::egl_native_display() const
{
//    return gbm.device;
    CALLED
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////
// Platform module entry points below
////////////////////////////////////////////////////////////////////////////////////

extern "C" std::shared_ptr<mg::Platform> mg::create_host_platform(
    std::shared_ptr<mo::Option> const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<DisplayReport> const& report)
{
    CALLED
    return std::make_shared<mgx::Platform>(report);
}

extern "C" std::shared_ptr<mg::Platform> create_guest_platform(
    std::shared_ptr<mg::DisplayReport> const& report,
    std::shared_ptr<mg::NestedContext> const&)
{
    CALLED
    return std::make_shared<mgx::Platform>(report);
}

extern "C" void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
    CALLED
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
    CALLED
    return mg::PlatformPriority::best;
}

mir::ModuleProperties const description = {
    "X",
    0,
    1,
    0
};

extern "C" mir::ModuleProperties const* describe_graphics_module()
{
    CALLED
    return &description;
}
