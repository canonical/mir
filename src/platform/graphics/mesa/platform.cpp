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
#include "buffer_allocator.h"
#include "display.h"
#include "internal_client.h"
#include "internal_native_display.h"
#include "linux_virtual_terminal.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/buffer_ipc_packer.h"
#include "mir/options/option.h"
#include "mir/graphics/native_buffer.h"

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

}

std::shared_ptr<mgm::InternalNativeDisplay> mgm::Platform::internal_native_display;
bool mgm::Platform::internal_display_clients_present;
mgm::Platform::Platform(std::shared_ptr<DisplayReport> const& listener,
                        std::shared_ptr<VirtualTerminal> const& vt)
    : udev{std::make_shared<UdevContext>()},
      listener{listener},
      vt{vt}
{
    drm.setup(udev);
    gbm.setup(drm);
    internal_display_clients_present = false;
}

mgm::Platform::~Platform()
{
    internal_native_display.reset();
    internal_display_clients_present = false;
}


std::shared_ptr<mg::GraphicBufferAllocator> mgm::Platform::create_buffer_allocator(
        const std::shared_ptr<mg::BufferInitializer>& buffer_initializer)
{
    return std::make_shared<mgm::BufferAllocator>(gbm.device, buffer_initializer);
}

std::shared_ptr<mg::Display> mgm::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy)
{
    return std::make_shared<mgm::Display>(
        this->shared_from_this(),
        initial_conf_policy,
        listener);
}

std::shared_ptr<mg::PlatformIPCPackage> mgm::Platform::get_ipc_package()
{
    return std::make_shared<MesaPlatformIPCPackage>(drm.get_authenticated_fd());
}

void mgm::Platform::fill_ipc_package(BufferIPCPacker* packer, Buffer const* buffer) const
{
    auto native_handle = buffer->native_buffer_handle();
    for(auto i=0; i<native_handle->data_items; i++)
    {
        packer->pack_data(native_handle->data[i]);
    }
    for(auto i=0; i<native_handle->fd_items; i++)
    {
        packer->pack_fd(native_handle->fd[i]);
    }

    packer->pack_stride(buffer->stride());
    packer->pack_flags(native_handle->flags);
    packer->pack_size(buffer->size());
}

void mgm::Platform::drm_auth_magic(drm_magic_t magic)
{
    drm.auth_magic(magic);
}

std::shared_ptr<mg::InternalClient> mgm::Platform::create_internal_client()
{
    if (!internal_native_display)
        internal_native_display = std::make_shared<mgm::InternalNativeDisplay>(get_ipc_package());
    internal_display_clients_present = true;
    return std::make_shared<mgm::InternalClient>(internal_native_display);
}

EGLNativeDisplayType mgm::Platform::egl_native_display() const
{
    return gbm.device;
}

extern "C" std::shared_ptr<mg::Platform> mg::create_platform(std::shared_ptr<mo::Option> const& options, std::shared_ptr<DisplayReport> const& report)
{
    auto real_fops = std::make_shared<RealVTFileOperations>();
    auto vt = std::make_shared<mgm::LinuxVirtualTerminal>(real_fops, options->get("vt", 0), report);
    return std::make_shared<mgm::Platform>(report, vt);
}

extern "C" int mir_server_mesa_egl_native_display_is_valid(MirMesaEGLNativeDisplay* display)
{
    return ((mgm::Platform::internal_display_clients_present) &&
            (display == mgm::Platform::internal_native_display.get()));
}
