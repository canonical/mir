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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/compositor/proxy_buffer.h"
#include "mir/compositor/buffer_id.h"

namespace mc=mir::compositor;
namespace geom=mir::geometry;

mc::ProxyBuffer::ProxyBuffer(std::weak_ptr<mc::Buffer> buffer)
 : buffer(buffer)
{
}
/*
std::shared_ptr<mc::Buffer> mc::ProxyBuffer::acquire_buffer_ownership()
{

}
*/

geom::Size mc::ProxyBuffer::size() const
{
    return geom::Size();
}

geom::Stride mc::ProxyBuffer::stride() const
{
    return geom::Stride();
}

geom::PixelFormat mc::ProxyBuffer::pixel_format() const
{
    return geom::PixelFormat();
}

void mc::ProxyBuffer::bind_to_texture()
{
}

std::shared_ptr<mc::BufferIPCPackage> mc::ProxyBuffer::get_ipc_package() const
{
    return std::shared_ptr<mc::BufferIPCPackage>();
}

mc::BufferID mc::ProxyBuffer::id() const
{
    return mc::BufferID(2);
}
