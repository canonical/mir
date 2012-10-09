/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

/* note: (kdub) this whole file is pretty much a TODO. if we do client side allocations
         in gbm, the factory can actually allocate buffers here */

#include "mir_client/mir_client_library.h"
#include "mir_connection.h"
#include "client_buffer_factory.h"
#include "client_buffer.h"

namespace mcl=mir::client;
namespace geom=mir::geometry;

class EmptyBuffer : public mcl::ClientBuffer
{
    std::shared_ptr<mcl::MemoryRegion> secure_for_cpu_write()
    {
        return std::make_shared<mcl::MemoryRegion>();
    };
    geom::Width width() const
    {
        return geom::Width{0};
    }
    geom::Height height() const
    {
        return geom::Height{0};
    }
    geom::PixelFormat pixel_format() const
    {
        return geom::PixelFormat::rgba_8888;
    }
};

/* todo: replace with real buffer factory */
class EmptyFactory : public mcl::ClientBufferFactory
{
public:
    std::shared_ptr<mcl::ClientBuffer> create_buffer_from_ipc_message(const std::shared_ptr<MirBufferPackage>&,
                                geom::Width, geom::Height, geom::PixelFormat) 
    {
        return std::make_shared<EmptyBuffer>();
    }
};

std::shared_ptr<mcl::ClientBufferFactory> mcl::create_platform_factory()
{
    return std::make_shared<EmptyFactory>();
}
