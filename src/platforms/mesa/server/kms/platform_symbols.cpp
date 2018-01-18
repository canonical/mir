/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "platform.h"
#include "gbm_platform.h"
#include "linux_virtual_terminal.h"
#include "mir/options/program_option.h"
#include "mir/options/option.h"
#include "mir/udev/wrapper.h"
#include "mir/module_deleter.h"
#include "mir/assert_module_entry_point.h"
#include "mir/libname.h"

#include <EGL/egl.h>
#include <fcntl.h>
#include <sys/ioctl.h>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace mo = mir::options;

namespace
{
char const* bypass_option_name{"bypass"};
char const* vt_option_name{"vt"};
char const* host_socket{"host-socket"};

struct RealVTFileOperations : public mgm::VTFileOperations
{
    int open(char const* pathname, int flags)
    {
        return ::open(pathname, flags);
    }

    int close(int fd)
    {
        return ::close(fd);
    }

    int ioctl(int d, int request, int val)
    {
        return ::ioctl(d, request, val);
    }

    int ioctl(int d, int request, void* p_val)
    {
        return ::ioctl(d, request, p_val);
    }

    int tcsetattr(int d, int acts, const struct termios *tcattr)
    {
        return ::tcsetattr(d, acts, tcattr);
    }

    int tcgetattr(int d, struct termios *tcattr)
    {
        return ::tcgetattr(d, tcattr);
    }
};

struct RealPosixProcessOperations : public mgm::PosixProcessOperations
{
    pid_t getpid() const override
    {
        return ::getpid();
    }
    pid_t getppid() const override
    {
        return ::getppid();
    }
    pid_t getpgid(pid_t process) const override
    {
        return ::getpgid(process);
    }
    pid_t getsid(pid_t process) const override
    {
        return ::getsid(process);
    }
    int setpgid(pid_t process, pid_t group) override
    {
        return ::setpgid(process, group);
    }
    pid_t setsid() override
    {
        return ::setsid();
    }
};
}

mir::UniqueModulePtr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mg::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const& /*logger*/)
{
    mir::assert_entry_point_signature<mg::CreateHostPlatform>(&create_host_platform);
    // ensure mesa finds the mesa mir-platform symbols
    auto real_fops = std::make_shared<RealVTFileOperations>();
    auto real_pops = std::unique_ptr<RealPosixProcessOperations>(new RealPosixProcessOperations{});
    auto vt = std::make_shared<mgm::LinuxVirtualTerminal>(
        real_fops,
        std::move(real_pops),
        options->get<int>(vt_option_name),
        report);

    auto bypass_option = mgm::BypassOption::allowed;
    if (!options->get<bool>(bypass_option_name))
        bypass_option = mgm::BypassOption::prohibited;

    return mir::make_module_ptr<mgm::Platform>(
        report, vt, *emergency_cleanup_registry, bypass_option);
}

void add_graphics_platform_options(boost::program_options::options_description& config)
{
    mir::assert_entry_point_signature<mg::AddPlatformOptions>(&add_graphics_platform_options);
    config.add_options()
        (vt_option_name,
         boost::program_options::value<int>()->default_value(0),
         "[platform-specific] VT to run on or 0 to use current.")
        (bypass_option_name,
         boost::program_options::value<bool>()->default_value(true),
         "[platform-specific] utilize the bypass optimization for fullscreen surfaces.");
}

mg::PlatformPriority probe_graphics_platform(mo::ProgramOption const& options)
{
    mir::assert_entry_point_signature<mg::PlatformProbe>(&probe_graphics_platform);
    auto const unparsed_arguments = options.unparsed_command_line();
    auto platform_option_used = false;

    for (auto const& token : unparsed_arguments)
    {
        if (token == (std::string("--") + vt_option_name))
            platform_option_used = true;
    }

    if (options.is_set(vt_option_name))
        platform_option_used = true;
    auto nested = options.is_set(host_socket);

    auto udev = std::make_shared<mir::udev::Context>();

    mir::udev::Enumerator drm_devices{udev};
    drm_devices.match_subsystem("drm");
    drm_devices.match_sysname("card[0-9]*");
    drm_devices.scan_devices();

    if (drm_devices.begin() == drm_devices.end())
        return mg::PlatformPriority::unsupported;

    // We also require GBM EGL platform
    auto const* client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
    if (!client_extensions)
    {
        // Doesn't support EGL client extensions; Mesa does, so this is unlikely to be mesa.
        return mg::PlatformPriority::unsupported;
    }
    if (strstr(client_extensions, "EGL_MESA_platform_gbm") == nullptr)
    {
        // No platform_gbm support, so we can't work.
        return mg::PlatformPriority::unsupported;
    }

    // Check for master
    int tmp_fd = -1;
    for (auto& device : drm_devices)
    {
        if (device.devnode() == nullptr)
        {
            /* The display connectors attached to the card appear as subdevices
             * of the card[0-9] node.
             * These won't have a device node, so pass on anything that doesn't have
             * a /dev/dri/card* node
             */
            continue;
        }
        tmp_fd = open(device.devnode(), O_RDWR | O_CLOEXEC);
        if (tmp_fd >= 0)
            break;
    }

    if (nested && platform_option_used)
        return mg::PlatformPriority::best;
    if (nested)
        return mg::PlatformPriority::supported;

    if (tmp_fd >= 0)
    {
        if (drmSetMaster(tmp_fd) >= 0)
        {
            drmDropMaster(tmp_fd);
            drmClose(tmp_fd);
            return mg::PlatformPriority::best;
        }
        else
            drmClose(tmp_fd);
    }

    if (platform_option_used)
        return mg::PlatformPriority::best;

    /* We failed to set mastership. However, still on most systems mesa-kms
     * is the right driver to choose. Landing here just means the user did
     * not specify --vt or is running from ssh. Still in most cases mesa-kms
     * is the correct option so give it a go. Better to fail trying to switch
     * VTs (and tell the user that) than to refuse to load the correct
     * driver at all. (LP: #1528082)
     *
     * Just make sure we are below PlatformPriority::supported in case
     * mesa-x11 can be used instead.
     *
     * TODO: Revisit the priority terminology. having a range of values between
     *       "supported" and "unsupported" is potentially confusing.
     * TODO: Revisit the return code of this function. We document that
     *       integer values outside the enum are allowed but C++ disallows it
     *       without a cast. So we should return an int or typedef to int
     *       instead.
     */
    return static_cast<mg::PlatformPriority>(
        mg::PlatformPriority::supported - 1);
}

namespace
{
mir::ModuleProperties const description = {
    "mir:mesa-kms",
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

mir::UniqueModulePtr<mir::graphics::DisplayPlatform> create_display_platform(
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<mg::DisplayReport> const& report,
    std::shared_ptr<mir::logging::Logger> const&)
{
    mir::assert_entry_point_signature<mg::CreateDisplayPlatform>(&create_display_platform);
    // ensure mesa finds the mesa mir-platform symbols
    auto real_fops = std::make_shared<RealVTFileOperations>();
    auto real_pops = std::unique_ptr<RealPosixProcessOperations>(new RealPosixProcessOperations{});
    auto vt = std::make_shared<mgm::LinuxVirtualTerminal>(
        real_fops,
        std::move(real_pops),
        options->get<int>(vt_option_name),
        report);

    auto bypass_option = mgm::BypassOption::allowed;
    if (!options->get<bool>(bypass_option_name))
        bypass_option = mgm::BypassOption::prohibited;

    return mir::make_module_ptr<mgm::Platform>(
        report, vt, *emergency_cleanup_registry, bypass_option);
}

mir::UniqueModulePtr<mir::graphics::RenderingPlatform> create_rendering_platform(
    std::shared_ptr<mir::options::Option> const& options,
    std::shared_ptr<mir::graphics::PlatformAuthentication> const& platform_authentication)
{
    mir::assert_entry_point_signature<mg::CreateRenderingPlatform>(&create_rendering_platform);

    auto bypass_option = mgm::BypassOption::allowed;
    if (options->is_set(bypass_option_name) && !options->get<bool>(bypass_option_name))
        bypass_option = mgm::BypassOption::prohibited;
    return mir::make_module_ptr<mgm::GBMPlatform>(
        bypass_option, mgm::BufferImportMethod::gbm_native_pixmap, platform_authentication);
}
