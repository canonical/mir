/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "platform.h"
#include "native_platform.h"
#include "buffer_allocator.h"
#include "display.h"
#include "internal_client.h"
#include "internal_native_display.h"
#include "linux_virtual_terminal.h"
#include "buffer_writer.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/buffer_ipc_packer.h"
#include "mir/options/option.h"
#include "mir/graphics/native_buffer.h"
#include "mir/emergency_cleanup_registry.h"

#include "drm_close_threadsafe.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

#include <fcntl.h>
#include <sys/ioctl.h>

namespace mg = mir::graphics;
namespace mgm = mg::mesa;
namespace mo = mir::options;

namespace
{
char const* bypass_option_name{"bypass"};
char const* vt_option_name{"vt"};

struct MesaPlatformIPCPackage : public mg::PlatformIPCPackage
{
    MesaPlatformIPCPackage(int drm_auth_fd)
    {
        ipc_fds.push_back(drm_auth_fd);
    }

    ~MesaPlatformIPCPackage()
    {
        if (ipc_fds.size() > 0 && ipc_fds[0] >= 0)
            mgm::drm_close_threadsafe(ipc_fds[0]);
    }
};

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

std::shared_ptr<mgm::InternalNativeDisplay> mgm::Platform::internal_native_display;
bool mgm::Platform::internal_display_clients_present;
mgm::Platform::Platform(std::shared_ptr<DisplayReport> const& listener,
                        std::shared_ptr<VirtualTerminal> const& vt,
                        EmergencyCleanupRegistry& emergency_cleanup_registry,
                        BypassOption bypass_option)
    : udev{std::make_shared<mir::udev::Context>()},
      drm{std::make_shared<helpers::DRMHelper>()},
      listener{listener},
      vt{vt},
      bypass_option_{bypass_option}
{
    drm->setup(udev);
    gbm.setup(*drm);
    internal_display_clients_present = false;

    std::weak_ptr<VirtualTerminal> weak_vt = vt;
    std::weak_ptr<helpers::DRMHelper> weak_drm = drm;
    emergency_cleanup_registry.add(
        [weak_vt,weak_drm]
        {
            if (auto const vt = weak_vt.lock())
                try { vt->restore(); } catch (...) {}

            if (auto const drm = weak_drm.lock())
                try { drm->drop_master(); } catch (...) {}
        });
}

mgm::Platform::~Platform()
{
    internal_native_display.reset();
    internal_display_clients_present = false;
}


std::shared_ptr<mg::GraphicBufferAllocator> mgm::Platform::create_buffer_allocator(
        const std::shared_ptr<mg::BufferInitializer>& buffer_initializer)
{
    return std::make_shared<mgm::BufferAllocator>(gbm.device, buffer_initializer, bypass_option_);
}

std::shared_ptr<mg::Display> mgm::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
    std::shared_ptr<GLProgramFactory> const&,
    std::shared_ptr<GLConfig> const& gl_config)
{
    return std::make_shared<mgm::Display>(
        this->shared_from_this(),
        initial_conf_policy,
        gl_config,
        listener);
}

std::shared_ptr<mg::PlatformIPCPackage> mgm::Platform::get_ipc_package()
{
    return std::make_shared<MesaPlatformIPCPackage>(drm->get_authenticated_fd());
}

void mgm::Platform::fill_buffer_package(
    BufferIPCPacker* packer, Buffer const* buffer, BufferIpcMsgType msg_type) const
{
    if (msg_type == mg::BufferIpcMsgType::full_msg)
    {
        auto native_handle = buffer->native_buffer_handle();
        for(auto i=0; i<native_handle->data_items; i++)
        {
            packer->pack_data(native_handle->data[i]);
        }
        for(auto i=0; i<native_handle->fd_items; i++)
        {
            packer->pack_fd(mir::Fd(IntOwnedFd{native_handle->fd[i]}));
        }

        packer->pack_stride(buffer->stride());
        packer->pack_flags(native_handle->flags);
        packer->pack_size(buffer->size());
    }
}

void mgm::Platform::drm_auth_magic(unsigned int magic)
{
    drm->auth_magic(magic);
}

std::shared_ptr<mg::InternalClient> mgm::Platform::create_internal_client()
{
    if (!internal_native_display)
        internal_native_display = std::make_shared<mgm::InternalNativeDisplay>(get_ipc_package());
    internal_display_clients_present = true;
    return std::make_shared<mgm::InternalClient>(internal_native_display);
}

std::shared_ptr<mg::BufferWriter> mgm::Platform::make_buffer_writer()
{
    return std::make_shared<mgm::BufferWriter>();
}

EGLNativeDisplayType mgm::Platform::egl_native_display() const
{
    return gbm.device;
}

mgm::BypassOption mgm::Platform::bypass_option() const
{
    return bypass_option_;
}

extern "C" std::shared_ptr<mg::Platform> mg::create_platform(
    std::shared_ptr<mo::Option> const& options,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& emergency_cleanup_registry,
    std::shared_ptr<DisplayReport> const& report)
{
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

    return std::make_shared<mgm::Platform>(
        report, vt, *emergency_cleanup_registry, bypass_option);
}

extern "C" int mir_server_mesa_egl_native_display_is_valid(MirMesaEGLNativeDisplay* display)
{
    bool nested_internal_display_in_use = mgm::NativePlatform::internal_native_display_in_use();
    bool host_internal_display_in_use = mgm::Platform::internal_display_clients_present;

    if (host_internal_display_in_use)
        return (display == mgm::Platform::internal_native_display.get());
    else if (nested_internal_display_in_use)
        return (display == mgm::NativePlatform::internal_native_display().get());
    return 0;
}

extern "C" void add_platform_options(boost::program_options::options_description& config)
{
    config.add_options()
        (vt_option_name,
         boost::program_options::value<int>()->default_value(0),
         "[platform-specific] VT to run on or 0 to use current.")
        (bypass_option_name,
         boost::program_options::value<bool>()->default_value(true),
         "[platform-specific] utilize the bypass optimization for fullscreen surfaces.");
}
