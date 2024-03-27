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

#ifndef MIR_TEST_DOUBLES_MOCK_BUFFER_STREAM_H_
#define MIR_TEST_DOUBLES_MOCK_BUFFER_STREAM_H_

#include "mir/compositor/buffer_stream.h"
#include "stub_buffer.h"
#include <gmock/gmock.h>

namespace mir
{
namespace test
{
namespace doubles
{
struct MockBufferStream : public compositor::BufferStream
{
    int buffers_ready_{0};
    std::function<void(geometry::Size const&)> frame_posted_callback;
    int buffers_ready(void const*)
    {
        if (buffers_ready_)
            return buffers_ready_--;
        return 0;
    }

    MockBufferStream()
    {
        ON_CALL(*this, buffers_ready_for_compositor(::testing::_))
            .WillByDefault(testing::Invoke(this, &MockBufferStream::buffers_ready));
        ON_CALL(*this, acquire_client_buffer(testing::_))
            .WillByDefault(testing::InvokeArgument<0>(nullptr));
        ON_CALL(*this, has_submitted_buffer())
            .WillByDefault(testing::Return(true));
        ON_CALL(*this, pixel_format())
            .WillByDefault(testing::Return(mir_pixel_format_abgr_8888));
        ON_CALL(*this, stream_size())
            .WillByDefault(testing::Return(geometry::Size{0,0}));
        ON_CALL(*this, set_frame_posted_callback(testing::_))
            .WillByDefault(testing::Invoke([&](auto const& callback){ frame_posted_callback = callback; }));
    }
    std::shared_ptr<StubBuffer> buffer { std::make_shared<StubBuffer>() };
    MOCK_METHOD1(acquire_client_buffer, void(std::function<void(graphics::Buffer* buffer)>));
    MOCK_METHOD1(release_client_buffer, void(graphics::Buffer*));
    MOCK_METHOD1(lock_compositor_buffer,
                 std::shared_ptr<graphics::Buffer>(void const*));
    MOCK_METHOD1(set_frame_posted_callback, void(std::function<void(geometry::Size const&)> const&));

    MOCK_METHOD0(get_stream_pixel_format, MirPixelFormat());
    MOCK_METHOD0(stream_size, geometry::Size());
    MOCK_METHOD0(force_client_completion, void());

    MOCK_CONST_METHOD1(buffers_ready_for_compositor, int(void const*));
    MOCK_METHOD0(drop_client_requests, void());

    MOCK_METHOD(void, submit_buffer, (std::shared_ptr<graphics::Buffer> const&), (override));
    MOCK_METHOD(MirPixelFormat, pixel_format, (), (const override));
    MOCK_METHOD(bool, has_submitted_buffer, (), (const override));
    MOCK_METHOD(void, set_scale, (float), (override));

};
}
}
}

#endif /* MIR_TEST_DOUBLES_MOCK_BUFFER_STREAM_H_ */
