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
#include "xserver_connection.h"
#include "mir/udev/wrapper.h"

#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace mgx = mg::X;
namespace mo = mir::options;
namespace mx = mir::X;

//TODO: Remove this global by allowing input and graphics drivers to share
//     some context like the x11 display handle.
std::shared_ptr<mx::X11Connection> x11_connection;

mgx::Platform::Platform()
    : udev{std::make_shared<mir::udev::Context>()},
       drm{std::make_shared<mesa::helpers::DRMHelper>(mesa::helpers::DRMNodeToUse::render)}
{
    if (x11_connection)
        BOOST_THROW_EXCEPTION(std::runtime_error("Cannot create x11 platform more than once"));

    x11_connection.reset(new mx::X11Connection());

    if (!x11_connection->dpy)
        BOOST_THROW_EXCEPTION(std::runtime_error("Cannot open x11 display"));

    drm->setup(udev);
    gbm.setup(*drm);
}

std::shared_ptr<mg::GraphicBufferAllocator> mgx::Platform::create_buffer_allocator()
{
    return std::make_shared<mgm::BufferAllocator>(gbm.device, mgm::BypassOption::prohibited, mgm::BufferImportMethod::dma_buf);
}

std::shared_ptr<mg::Display> mgx::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
    std::shared_ptr<GLProgramFactory> const&,
    std::shared_ptr<GLConfig> const& /*gl_config*/)
{
    return std::make_shared<mgx::Display>(x11_connection->dpy);
}

std::shared_ptr<mg::PlatformIpcOperations> mgx::Platform::make_ipc_operations() const
{
    return std::make_shared<mg::mesa::IpcOperations>(drm);
}

EGLNativeDisplayType mgx::Platform::egl_native_display() const
{
    return eglGetDisplay(x11_connection->dpy);
}

////////////////////////////////////////////////////////////////////////////////////
// Platform module entry points below
////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<mg::DisplayReport> const& /*report*/)
{
    return std::make_shared<mgx::Platform>();
}

std::shared_ptr<mg::Platform> create_guest_platform(
    std::shared_ptr<mg::DisplayReport> const& /*report*/,
    std::shared_ptr<mg::NestedContext> const&)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Guest platform isn't supported under X"));
    return nullptr;
}

void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
}

mg::PlatformPriority probe_graphics_platform(mo::ProgramOption const& /*options*/)
{
    auto dpy = XOpenDisplay(nullptr);
    if (dpy)
    {
        XCloseDisplay(dpy);

        auto udev = std::make_shared<mir::udev::Context>();

        mir::udev::Enumerator drm_devices{udev};
        drm_devices.match_subsystem("drm");
        drm_devices.match_sysname("renderD[0-9]*");
        drm_devices.scan_devices();

        for (auto& device : drm_devices)
        {
            static_cast<void>(device);

            return mg::PlatformPriority::best;
        }
    }
    return mg::PlatformPriority::unsupported;
}

mir::ModuleProperties const description = {
    "mesa-x11",
    MIR_VERSION_MAJOR,
    MIR_VERSION_MINOR,
    MIR_VERSION_MICRO
};

mir::ModuleProperties const* describe_graphics_module()
{
    return &description;
}
