/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_DOUBLES_NULL_BUFFER_STREAM_H_
#define MIR_TEST_DOUBLES_NULL_BUFFER_STREAM_H_

#include <mir/compositor/buffer_stream.h>
#include <mir/test/doubles/stub_buffer.h>

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


    std::shared_ptr<graphics::Buffer> lock_compositor_buffer(void const*) override
    {
        --nready;
        return stub_compositor_buffer;
    }

    geometry::Size stream_size() override
    {
        return geometry::Size();
    }

    void submit_buffer(
        std::shared_ptr<graphics::Buffer> const& b,
        geometry::Size /*dst_size*/,
        geometry::RectangleD /*src_bounds*/) override
    {
        if (b) ++nready;
    }
    MirPixelFormat pixel_format() const override { return mir_pixel_format_abgr_8888; }
    void set_frame_posted_callback(std::function<void(geometry::Size const&)> const&) override {}
    bool has_submitted_buffer() const override { return true; }
    void set_scale(float) override {}

    std::shared_ptr<graphics::Buffer> stub_compositor_buffer;
    int nready = 0;
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_NULL_BUFFER_STREAM_H_ */
