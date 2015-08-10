/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/fake_shared.h"
#include "src/server/compositor/stream.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/frontend/client_buffers.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
using namespace testing;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace
{
struct MockSurfaceObserver : mir::scene::NullSurfaceObserver
{
    MOCK_METHOD1(frame_posted, void(int));
};

struct StubBufferMap : mf::ClientBuffers
{
    StubBufferMap(mf::EventSink& sink, std::vector<std::shared_ptr<mg::Buffer>>& buffers) :
        buffers{buffers},
        sink{sink}
    {
    }
    void add_buffer(mg::BufferProperties const&)
    {
    }
    void remove_buffer(mg::BufferID)
    {
    }
    void send_buffer(mg::BufferID id)
    {
        sink.send_buffer(mf::BufferStreamId{33}, *operator[](id), mg::BufferIpcMsgType::update_msg);
    }
    std::shared_ptr<mg::Buffer>& operator[](mg::BufferID id)
    {
        auto it = std::find_if(buffers.begin(), buffers.end(),
            [id](std::shared_ptr<mg::Buffer> const& b)
            {
                return b->id() == id;
            });
        if (it == buffers.end())
            throw std::logic_error("cannot find buffer in map");
        return *it;
    }
    std::vector<std::shared_ptr<mg::Buffer>>& buffers;
    mf::EventSink& sink;
};

struct Stream : Test
{
    Stream() :
        buffers{
            std::make_shared<mtd::StubBuffer>(),
            std::make_shared<mtd::StubBuffer>(),
            std::make_shared<mtd::StubBuffer>()}
    {
    }
    
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    NiceMock<mtd::MockEventSink> mock_sink;
    geom::Size initial_size{44,2};
    MirPixelFormat construction_format{mir_pixel_format_rgb_565};
    mc::Stream stream{std::make_unique<StubBufferMap>(mock_sink, buffers), initial_size, construction_format};
};
}

TEST_F(Stream, swapping_returns_null_via_callback)
{
    stream.swap_buffers(buffers[0].get(), [](mg::Buffer* buffer) {
        EXPECT_THAT(buffer, IsNull());
    });
}

TEST_F(Stream, transitions_from_queuing_to_framedropping)
{
    EXPECT_CALL(mock_sink, send_buffer(_,_,_)).Times(buffers.size() - 1);
    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});
    stream.allow_framedropping(true);

    std::vector<std::shared_ptr<mg::Buffer>> cbuffers;
    while(stream.buffers_ready_for_compositor(this))
        cbuffers.push_back(stream.lock_compositor_buffer(this));
    ASSERT_THAT(cbuffers, SizeIs(1));
    EXPECT_THAT(cbuffers[0]->id(), Eq(buffers.back()->id()));
}

TEST_F(Stream, transitions_from_framedropping_to_queuing)
{
    stream.allow_framedropping(true);
    Mock::VerifyAndClearExpectations(&mock_sink);

    EXPECT_CALL(mock_sink, send_buffer(_,_,_)).Times(buffers.size() - 1);
    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});

    stream.allow_framedropping(false);
    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});

    Mock::VerifyAndClearExpectations(&mock_sink);

    std::vector<std::shared_ptr<mg::Buffer>> cbuffers;
    while(stream.buffers_ready_for_compositor(this))
        cbuffers.push_back(stream.lock_compositor_buffer(this));
    EXPECT_THAT(cbuffers, SizeIs(buffers.size()));
}

TEST_F(Stream, indicates_buffers_ready_when_queueing)
{
    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});

    for(auto i = 0u; i < buffers.size(); i++)
    {
        EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(1));
        stream.lock_compositor_buffer(this);
    }

    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(0));
}

TEST_F(Stream, indicates_buffers_ready_when_dropping)
{
    stream.allow_framedropping(true);

    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});

    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(1));
    stream.lock_compositor_buffer(this);
    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(0));
}

TEST_F(Stream, tracks_has_buffer)
{
    EXPECT_FALSE(stream.has_submitted_buffer());
    stream.swap_buffers(buffers[0].get(), [](mg::Buffer*){});
    EXPECT_TRUE(stream.has_submitted_buffer());
}

TEST_F(Stream, calls_observers_after_scheduling_on_submissions)
{
    auto observer = std::make_shared<MockSurfaceObserver>();
    EXPECT_CALL(*observer, frame_posted(1));
    stream.add_observer(observer);
    stream.swap_buffers(buffers[0].get(), [](mg::Buffer*){});
    stream.remove_observer(observer);
    stream.swap_buffers(buffers[0].get(), [](mg::Buffer*){});
}

TEST_F(Stream, calls_observers_call_doesnt_hold_lock)
{
    auto observer = std::make_shared<MockSurfaceObserver>();
    EXPECT_CALL(*observer, frame_posted(1))
        .WillOnce(InvokeWithoutArgs([&]{
            EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(1));
            EXPECT_TRUE(stream.has_submitted_buffer());
        }));
    stream.add_observer(observer);
    stream.swap_buffers(buffers[0].get(), [](mg::Buffer*){});
}

TEST_F(Stream, flattens_queue_out_when_told_to_drop)
{
    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});

    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(1));
    stream.drop_old_buffers();
    stream.lock_compositor_buffer(this);
    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(0));
}

TEST_F(Stream, ignores_nullptr_submissions) //legacy behavior
{
    auto observer = std::make_shared<MockSurfaceObserver>();
    EXPECT_CALL(*observer, frame_posted(_)).Times(0);
    stream.add_observer(observer);
    bool was_called = false;
    stream.swap_buffers(nullptr, [&](mg::Buffer*){ was_called = true; });
    EXPECT_FALSE(stream.has_submitted_buffer());
    EXPECT_TRUE(was_called);
}

//it doesnt quite make sense that the stream has a size, esp given that there could be different-sized buffers
//in the stream, and the surface has the onscreen size info
TEST_F(Stream, reports_size)
{
    geom::Size new_size{333,139};
    EXPECT_THAT(stream.stream_size(), Eq(initial_size));
    stream.resize(new_size);
    EXPECT_THAT(stream.stream_size(), Eq(new_size));
}

//Likewise, no reason buffers couldn't all be a different pixel format
TEST_F(Stream, reports_format)
{
    EXPECT_THAT(stream.pixel_format(), Eq(construction_format));
}
