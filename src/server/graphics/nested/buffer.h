/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_NESTED_BUFFER_H_
#define MIR_GRAPHICS_NESTED_BUFFER_H_

#include "mir_toolkit/client_types_nbs.h"
#include "mir/graphics/buffer_basic.h"
#include "mir/renderer/sw/pixel_source.h"
#include <memory>

namespace mir
{
namespace graphics
{
class BufferProperties;
namespace nested
{
class HostConnection;
class Buffer : public BufferBasic,
               public NativeBufferBase,
               public renderer::software::PixelSource
{
public:
    Buffer(std::shared_ptr<HostConnection> const& connection, BufferProperties const& properties);

    std::shared_ptr<NativeBuffer> native_buffer_handle() const override;
    geometry::Size size() const override;
    MirPixelFormat pixel_format() const override;
    NativeBufferBase* native_buffer_base() override;

    void write(unsigned char const* pixels, size_t size) override;
    void read(std::function<void(unsigned char const*)> const& do_with_pixels) override;
    geometry::Stride stride() const override;

private:
    std::shared_ptr<NativeBufferBase> create_native_base(BufferUsage const usage);

    std::shared_ptr<HostConnection> const connection;
    std::shared_ptr<MirBuffer> buffer;
    geometry::Stride const stride_;
    std::shared_ptr<NativeBufferBase> const native_base;
};
}
}
}
#endif /* MIR_GRAPHICS_NESTED_BUFFERH_H_ */
