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

#ifndef MIR_TEST_DOUBLES_MOCK_BUFFER_STREAM_H_
#define MIR_TEST_DOUBLES_MOCK_BUFFER_STREAM_H_

#include "mir/compositor/buffer_stream.h"

#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockBufferStream : public compositor::BufferStream
{
    MOCK_METHOD2(swap_client_buffers, void(graphics::Buffer*, std::function<void(graphics::Buffer*)> completee));
    MOCK_METHOD1(lock_compositor_buffer,
                 std::shared_ptr<graphics::Buffer>(unsigned long));
    MOCK_METHOD0(lock_snapshot_buffer, std::shared_ptr<graphics::Buffer>());

    MOCK_METHOD0(get_stream_pixel_format, MirPixelFormat());
    MOCK_METHOD0(stream_size, geometry::Size());
    MOCK_METHOD1(resize, void(geometry::Size const&));
    MOCK_METHOD0(force_client_completion, void());
    MOCK_METHOD1(allow_framedropping, void(bool));
    MOCK_METHOD0(force_requests_to_complete, void());
    int buffers_ready_for_compositor() const override { return 1; }
};
}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_BUFFER_STREAM_H_ */
