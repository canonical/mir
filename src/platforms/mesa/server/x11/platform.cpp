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

#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace mgx = mg::X;
namespace mo = mir::options;

::Display *x_display = nullptr;

mgx::Platform::Platform()
    : udev{std::make_shared<mir::udev::Context>()},
       drm{std::make_shared<mesa::helpers::DRMHelper>(true)}
{
   x_dpy = XOpenDisplay(NULL);
   if (!x_dpy)
       BOOST_THROW_EXCEPTION(std::runtime_error("Cannot open X display"));
   x_display = x_dpy;

   drm->setup(udev);
   gbm.setup(*drm);
}

mgx::Platform::~Platform()
{
    XCloseDisplay(x_dpy);
    x_display = nullptr;
}

std::shared_ptr<mg::GraphicBufferAllocator> mgx::Platform::create_buffer_allocator()
{
    return std::make_shared<mgm::BufferAllocator>(gbm.device, mgm::BypassOption::prohibited, true);
}

std::shared_ptr<mg::Display> mgx::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
    std::shared_ptr<GLProgramFactory> const&,
    std::shared_ptr<GLConfig> const& /*gl_config*/)
{
    return std::make_shared<mgx::Display>(x_dpy);
}

std::shared_ptr<mg::PlatformIpcOperations> mgx::Platform::make_ipc_operations() const
{
    return std::make_shared<mg::mesa::IpcOperations>(drm, false);
}

EGLNativeDisplayType mgx::Platform::egl_native_display() const
{
    return eglGetDisplay(x_dpy);
}

////////////////////////////////////////////////////////////////////////////////////
// Platform module entry points below
////////////////////////////////////////////////////////////////////////////////////

extern "C" std::shared_ptr<mg::Platform> mg::create_host_platform(
    std::shared_ptr<mo::Option> const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<DisplayReport> const& /*report*/)
{
    return std::make_shared<mgx::Platform>();
}

extern "C" std::shared_ptr<mg::Platform> create_guest_platform(
    std::shared_ptr<mg::DisplayReport> const& /*report*/,
    std::shared_ptr<mg::NestedContext> const&)
{
    BOOST_THROW_EXCEPTION(std::runtime_error("Guest platform isn't supported under X"));
    return nullptr;
}

extern "C" void add_graphics_platform_options(boost::program_options::options_description& /*config*/)
{
}

extern "C" mg::PlatformPriority probe_graphics_platform()
{
    auto dpy = XOpenDisplay(NULL);
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

extern "C" mir::ModuleProperties const* describe_graphics_module()
{
    return &description;
}
