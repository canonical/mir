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

#include "client_platform_factory.h"
#include "client_platform.h"
#include "buffer_file_ops.h"
#include "../egl_native_display_container.h"

#include <sys/mman.h>
#include <unistd.h>

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

std::shared_ptr<mcl::ClientPlatform>
mclm::ClientPlatformFactory::create_client_platform(mcl::ClientContext* context)
{
    auto buffer_file_ops = std::make_shared<RealBufferFileOps>();
    return std::make_shared<mclm::ClientPlatform>(
        context, buffer_file_ops, mcl::EGLNativeDisplayContainer::instance());
}

extern "C" std::shared_ptr<mcl::ClientPlatformFactory> mcl::create_client_platform_factory()
{
    return std::make_shared<mclm::ClientPlatformFactory>();
}
