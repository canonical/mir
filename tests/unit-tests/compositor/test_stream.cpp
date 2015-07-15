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
#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/fake_shared.h"
#include "src/server/compositor/stream.h"
#include "mir/scene/null_surface_observer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
using namespace testing;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{
struct MockSurfaceObserver : mir::scene::NullSurfaceObserver
{
    MOCK_METHOD1(frame_posted, void(int));
};

struct StreamTest : Test
{
    StreamTest() :
        buffers{
            std::make_shared<mtd::StubBuffer>(),
            std::make_shared<mtd::StubBuffer>(),
            std::make_shared<mtd::StubBuffer>()}
    {
    }

    mf::BufferStreamId id;
    mtd::MockEventSink mock_sink;
    mc::Stream stream{id, mt::fake_shared(mock_sink)};

    std::vector<std::shared_ptr<mg::Buffer>> buffers;
};
}

TEST_F(StreamTest, swapping_doesnt_make_sense_in_the_new_world)
{
    stream.swap_buffers(buffers[0].get(), [](mg::Buffer* buffer) {
        EXPECT_THAT(buffer, IsNull());
    });
}

TEST_F(StreamTest, transitions_from_queuing_to_framedropping)
{
    EXPECT_CALL(mock_sink, send_buffer(_,_,_)).Times(buffers.size() - 1);
    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});
    stream.allow_framedropping(true);
}

TEST_F(StreamTest, transitions_from_framedropping_to_queuing)
{
    EXPECT_CALL(mock_sink, send_buffer(_,_,_)).Times(buffers.size() * 2 - 1);
    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});
    stream.allow_framedropping(true);

    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});

    stream.allow_framedropping(false);
    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});
}

TEST_F(StreamTest, flubs_buffers_ready_for_queueing)
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

TEST_F(StreamTest, flubs_buffers_ready_for_dropping)
{
    stream.allow_framedropping(true);

    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});

    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(1));
    stream.lock_compositor_buffer(this);
    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(0));
}

TEST_F(StreamTest, tracks_has_buffer)
{
    EXPECT_FALSE(stream.has_submitted_buffer());
    stream.swap_buffers(buffers[0].get(), [](mg::Buffer*){});
    EXPECT_TRUE(stream.has_submitted_buffer());
}

TEST_F(StreamTest, calls_observers_on_submissions)
{
    auto observer = std::make_shared<MockSurfaceObserver>();
    EXPECT_CALL(*observer, frame_posted(1));
    stream.add_observer(observer);
    stream.swap_buffers(buffers[0].get(), [](mg::Buffer*){});
    stream.remove_observer(observer);
    stream.swap_buffers(buffers[0].get(), [](mg::Buffer*){});
}

TEST_F(StreamTest, flattens_queue)
{
    for(auto& buffer : buffers)
        stream.swap_buffers(buffer.get(), [](mg::Buffer*){});

    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(1));
    stream.drop_old_buffers();
    EXPECT_THAT(stream.buffers_ready_for_compositor(this), Eq(0));
}
