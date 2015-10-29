/*
 * Copyright © 2014 Canonical Ltd.
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

#include "mir/client_platform_factory.h"
#include "client_platform.h"
#include "mir_toolkit/client_types.h"
#include "mir/client_context.h"
#include "buffer_file_ops.h"
#include "mir/egl_native_display_container.h"

#include <sys/mman.h>
#include <unistd.h>
#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace mclm = mcl::mesa;

namespace
{

struct RealBufferFileOps : public mclm::BufferFileOps
{
    int close(int fd) const
    {
        while (::close(fd) == -1)
        {
            // Retry on EINTR, return error on anything else
            if (errno != EINTR)
                return errno;
        }
        return 0;
    }

    void* map(int fd, off_t offset, size_t size) const
    {
        return mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                    fd, offset);
    }

    void unmap(void* addr, size_t size) const
    {
        munmap(addr, size);
    }
};

}

std::shared_ptr<mcl::ClientPlatform> create_client_platform(mcl::ClientContext* context)
{
    MirPlatformPackage package;
    context->populate_server_package(package);
    if (package.data_items != 0 || package.fd_items != 1)
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"Attempted to create Mesa client platform on non-Mesa server"}));
    }
    auto buffer_file_ops = std::make_shared<RealBufferFileOps>();
    return std::make_shared<mclm::ClientPlatform>(
        context, buffer_file_ops, mcl::EGLNativeDisplayContainer::instance());
}

bool
is_appropriate_module(mcl::ClientContext* context)
{
    MirPlatformPackage platform;
    context->populate_server_package(platform);
    // TODO: Actually check what platform we're using, rather than blindly
    //       hope we can distinguish them from the stuff they've put in the
    //       PlatformPackage.
    return platform.data_items == 0 && platform.fd_items == 1;
}
