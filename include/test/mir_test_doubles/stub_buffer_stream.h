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

#include <mir/surfaces/buffer_stream.h>
#include <mir_test_doubles/stub_buffer.h>

namespace mir
{
namespace test
{
namespace doubles
{

class StubBufferStream : public surfaces::BufferStream
{
public:
    StubBufferStream()
    {
        stub_client_buffer = std::make_shared<StubBuffer>();
        stub_compositor_buffer = std::make_shared<StubBuffer>();
    }
    std::shared_ptr<graphics::Buffer> secure_client_buffer()
    {
        return stub_client_buffer;
    }

    std::shared_ptr<graphics::Buffer> lock_compositor_buffer(unsigned long)
    {
        return stub_compositor_buffer;
    }

    std::shared_ptr<graphics::Buffer> lock_snapshot_buffer()
    {
        return stub_compositor_buffer;
    }

    geometry::PixelFormat get_stream_pixel_format()
    {
        return geometry::PixelFormat();
    }

    geometry::Size stream_size()
    {
        return geometry::Size();
    }

    void force_requests_to_complete()
    {
    }

    void allow_framedropping(bool)
    {
    }

    std::shared_ptr<graphics::Buffer> stub_client_buffer;
    std::shared_ptr<graphics::Buffer> stub_compositor_buffer;
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_NULL_BUFFER_STREAM_H_ */
