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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_COMPOSITOR_TEMPORARY_BUFFERS_H_
#define MIR_COMPOSITOR_TEMPORARY_BUFFERS_H_

#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_id.h"

namespace mir
{
namespace compositor
{

class BufferID;
class BufferSwapper;

class TemporaryBuffer : public Buffer
{
public:
    geometry::Size size() const;
    geometry::Stride stride() const;
    geometry::PixelFormat pixel_format() const;
    BufferID id() const;
    void bind_to_texture();
    std::shared_ptr<BufferIPCPackage> get_ipc_package() const;

protected:
    explicit TemporaryBuffer(std::shared_ptr<Buffer> const& real_buffer);
    std::shared_ptr<Buffer> const buffer;
};

class TemporaryClientBuffer : public TemporaryBuffer
{
public:
    explicit TemporaryClientBuffer(std::shared_ptr<BufferSwapper> const& buffer_swapper);
    ~TemporaryClientBuffer();

private:
    std::weak_ptr<BufferSwapper> const allocating_swapper;
};

class TemporaryCompositorBuffer : public TemporaryBuffer
{
public:
    explicit TemporaryCompositorBuffer(std::shared_ptr<BufferSwapper> const& buffer_swapper);
    ~TemporaryCompositorBuffer();

private:
    std::weak_ptr<BufferSwapper> const allocating_swapper;
};

}
}

#endif /* MIR_COMPOSITOR_TEMPORARY_BUFFERS_H_ */
