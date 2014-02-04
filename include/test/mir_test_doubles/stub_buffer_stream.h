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

#ifndef MIR_TEST_DOUBLES_NULL_BUFFER_STREAM_H_
#define MIR_TEST_DOUBLES_NULL_BUFFER_STREAM_H_

#include <mir/compositor/buffer_stream.h>
#include <mir_test_doubles/stub_buffer.h>

namespace mir
{
namespace test
{
namespace doubles
{

class StubBufferStream : public compositor::BufferStream
{
public:
    StubBufferStream()
    {
        stub_compositor_buffer = std::make_shared<StubBuffer>();
    }
    void swap_client_buffers(graphics::Buffer*, std::function<void(graphics::Buffer* new_buffer)> complete) override
    {
        complete(&stub_client_buffer);
    }
    std::shared_ptr<graphics::Buffer> lock_compositor_buffer(unsigned long) override
    {
        return stub_compositor_buffer;
    }

    std::shared_ptr<graphics::Buffer> lock_snapshot_buffer() override
    {
        return stub_compositor_buffer;
    }

    MirPixelFormat get_stream_pixel_format() override
    {
        return MirPixelFormat();
    }

    geometry::Size stream_size() override
    {
        return geometry::Size();
    }

    void resize(geometry::Size const&) override
    {
    }

    void force_requests_to_complete() override
    {
    }

    void allow_framedropping(bool) override
    {
    }

    StubBuffer stub_client_buffer;
    std::shared_ptr<graphics::Buffer> stub_compositor_buffer;
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_NULL_BUFFER_STREAM_H_ */
