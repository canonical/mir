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

#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "src/server/compositor/buffer_schedule.h"

#include <gtest/gtest.h>
using namespace testing;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mf = mir::frontend;
namespace geom = mir::geometry;

namespace
{
struct MockBufferAllocator : public mg::GraphicBufferAllocator
{
    MOCK_METHOD1(alloc_buffer, std::shared_ptr<mg::Buffer>(mg::BufferProperties const&));
    MOCK_METHOD0(supported_pixel_formats, std::vector<MirPixelFormat>());
};

struct BufferSchedule : public Test
{
    MockBufferAllocator mock_allocator;
    mtd::StubBufferAllocator stub_allocator;
    mtd::MockEventSink mock_sink;
    mf::BufferStreamId stream_id{44};
    mc::BufferSchedule schedule{stream_id, mt::fake_shared(mock_sink)};
    mg::BufferProperties properties{geom::Size{42,43}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware}; 
};
}

TEST_F(BufferSchedule, sends_full_buffer_on_allocation)
{
    auto stub_buffer = std::make_unique<mtd::StubBuffer>();
    auto buffer_id = stub_buffer->id();

    mc::BufferSchedule schedule{stream_id, mt::fake_shared(mock_sink)};
    EXPECT_CALL(mock_sink, send_buffer(stream_id, Ref(*stub_buffer), mg::BufferIpcMsgType::full_msg));
    schedule.add_buffer(std::move(stub_buffer));
    schedule.remove_buffer(buffer_id);
}

TEST_F(BufferSchedule, compositor_access_before_any_submission_throws)
{
    mc::BufferSchedule schedule{stream_id, mt::fake_shared(mock_sink)};
    EXPECT_THROW({
        schedule.lock_compositor_buffer(this);
    }, std::logic_error);
    schedule.add_buffer(std::make_unique<mtd::StubBuffer>());
    EXPECT_THROW({
        schedule.lock_compositor_buffer(this);
    }, std::logic_error);
}

TEST_F(BufferSchedule, compositor_access)
{
    auto stub_buffer = std::make_unique<mtd::StubBuffer>();
    auto buffer_id = stub_buffer->id();
    mc::BufferSchedule schedule{stream_id, mt::fake_shared(mock_sink)};
    schedule.add_buffer(std::move(stub_buffer));
    schedule.schedule_buffer(buffer_id);
    auto cbuffer = schedule.lock_compositor_buffer(this);
    EXPECT_THAT(cbuffer->id(), Eq(buffer_id));
}

TEST_F(BufferSchedule, compositor_release_sends_buffer_back)
{
    auto stub_buffer = std::make_unique<mtd::StubBuffer>();
    auto buffer_id = stub_buffer->id();

    EXPECT_CALL(mock_sink, send_buffer(stream_id, Ref(*stub_buffer), mg::BufferIpcMsgType::update_msg));

    mc::BufferSchedule schedule{stream_id, mt::fake_shared(mock_sink)};
    schedule.add_buffer(std::move(stub_buffer));
    schedule.schedule_buffer(buffer_id);

    auto cbuffer = schedule.lock_compositor_buffer(this);
    cbuffer.reset();
}

TEST_F(BufferSchedule, compositor_can_acquire_different_buffers_if_submission_happens)
{
    mc::BufferSchedule schedule{stream_id, mt::fake_shared(mock_sink)};
    auto buffer = std::make_unique<mtd::StubBuffer>();
    auto buffer_id1 = buffer->id();
    schedule.add_buffer(std::move(buffer));
    buffer = std::make_unique<mtd::StubBuffer>();
    auto buffer_id2 = buffer->id();
    schedule.add_buffer(std::move(buffer));

    schedule.schedule_buffer(buffer_id1);
    auto cbuffer1 = schedule.lock_compositor_buffer(this);
    schedule.schedule_buffer(buffer_id2);
    auto cbuffer2 = schedule.lock_compositor_buffer(this);
    EXPECT_THAT(cbuffer1, Ne(cbuffer2));

    InSequence seq;
    EXPECT_CALL(mock_sink, send_buffer(_, Ref(*cbuffer2), _));
    EXPECT_CALL(mock_sink, send_buffer(_, Ref(*cbuffer1), _));
}

#if 0
//good idea?
TEST_F(BufferSchedule, submitted_buffer_can_be_ejected)
{
    schedule.schedule_buffer(buffer2);
    schedule.schedule_buffer(buffer1);
    schedule.eject(buffer1);

    EXPECT_THAT(schedule.lock_compositor_buffer(id), Eq(buffer2));
}
#endif
