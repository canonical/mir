/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "gbm_platform.h"
#include "gbm_buffer_allocator.h"
#include "gbm_display.h"
#include "internal_client.h"
#include "internal_native_display.h"
#include "linux_virtual_terminal.h"
#include "udev_video_devices.h"
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
namespace mgg = mg::gbm;
namespace mo = mir::options;

namespace
{

struct GBMPlatformIPCPackage : public mg::PlatformIPCPackage
{
    GBMPlatformIPCPackage(int drm_auth_fd)
    {
        ipc_fds.push_back(drm_auth_fd);
    }

    ~GBMPlatformIPCPackage()
    {
        if (ipc_fds.size() > 0 && ipc_fds[0] >= 0)
            mgg::drm_close_threadsafe(ipc_fds[0]);
    }
};

struct RealVTFileOperations : public mgg::VTFileOperations
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

std::shared_ptr<mgg::InternalNativeDisplay> mgg::GBMPlatform::internal_native_display;
bool mgg::GBMPlatform::internal_display_clients_present;
mgg::GBMPlatform::GBMPlatform(std::shared_ptr<DisplayReport> const& listener,
                              std::shared_ptr<VirtualTerminal> const& vt)
    : listener{listener},
      vt{vt}
{
    drm.setup(udev);
    gbm.setup(drm);
    internal_display_clients_present = false;
}

mgg::GBMPlatform::~GBMPlatform()
{
    internal_native_display.reset();
    internal_display_clients_present = false;
}


std::shared_ptr<mg::GraphicBufferAllocator> mgg::GBMPlatform::create_buffer_allocator(
        const std::shared_ptr<mg::BufferInitializer>& buffer_initializer)
{
    return std::make_shared<mgg::GBMBufferAllocator>(gbm.device,
                                                     buffer_initializer);
}

std::shared_ptr<mg::Display> mgg::GBMPlatform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy)
{
    return std::make_shared<mgg::GBMDisplay>(
        this->shared_from_this(),
        std::make_shared<UdevVideoDevices>(udev.ctx),
        initial_conf_policy,
        listener);
}

std::shared_ptr<mg::PlatformIPCPackage> mgg::GBMPlatform::get_ipc_package()
{
    return std::make_shared<GBMPlatformIPCPackage>(drm.get_authenticated_fd());
}

void mgg::GBMPlatform::fill_ipc_package(std::shared_ptr<BufferIPCPacker> const& packer,
                                        std::shared_ptr<Buffer> const& buffer) const
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
}

void mgg::GBMPlatform::drm_auth_magic(drm_magic_t magic)
{
    drm.auth_magic(magic);
}

std::shared_ptr<mg::InternalClient> mgg::GBMPlatform::create_internal_client()
{
    if (!internal_native_display)
        internal_native_display = std::make_shared<mgg::InternalNativeDisplay>(get_ipc_package()); 
    internal_display_clients_present = true;
    return std::make_shared<mgg::InternalClient>(internal_native_display);
}

extern "C" std::shared_ptr<mg::Platform> mg::create_platform(std::shared_ptr<mo::Option> const& options, std::shared_ptr<DisplayReport> const& report)
{
    auto real_fops = std::make_shared<RealVTFileOperations>();
    auto vt = std::make_shared<mgg::LinuxVirtualTerminal>(real_fops, options->get("vt", 0), report);
    return std::make_shared<mgg::GBMPlatform>(report, vt);
}

extern "C" int mir_server_mesa_egl_native_display_is_valid(MirMesaEGLNativeDisplay* display)
{
    return ((mgg::GBMPlatform::internal_display_clients_present) &&
            (display == mgg::GBMPlatform::internal_native_display.get()));
}
