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

#ifndef MIR_COMPOSITOR_PROXY_BUFFER_H_
#define MIR_COMPOSITOR_PROXY_BUFFER_H_

#include "mir/compositor/buffer.h"

namespace mir
{
namespace compositor
{

class ProxyBuffer : public Buffer
{
public:
    explicit ProxyBuffer(std::weak_ptr<Buffer> buffer);

    geometry::Size size() const;
    geometry::Stride stride() const;
    geometry::PixelFormat pixel_format() const;
    void bind_to_texture();

    std::shared_ptr<BufferIPCPackage> get_ipc_package() const;
    BufferID id() const;

private:
    std::shared_ptr<Buffer> acquire_buffer_ownership() const;
    std::weak_ptr<Buffer> buffer;
}; 

}
}

#endif /* MIR_COMPOSITOR_PROXY_BUFFER_H_ */
