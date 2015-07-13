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
#include "src/server/compositor/buffer_map.h"
#include "mir/graphics/display_configuration.h"

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

struct StubBufferAllocator : public mg::GraphicBufferAllocator
{
    std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const&)
    {
        auto b = std::make_shared<mtd::StubBuffer>();
        map[b->id()] = b;
        ids.push_back(b->id());
        return b;
    }
    std::vector<MirPixelFormat> supported_pixel_formats()
    {
        return {};
    }
    std::map<mg::BufferID, std::shared_ptr<mg::Buffer>> map;
    std::vector<mg::BufferID> ids;
};
}

struct ClientBuffers : public Test
{
    testing::NiceMock<mtd::MockEventSink> mock_sink;
    mf::BufferStreamId stream_id{44};
    mg::BufferProperties properties{geom::Size{42,43}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    MockBufferAllocator mock_allocator;
    StubBufferAllocator stub_allocator;
    mc::BufferMap map{stream_id, mt::fake_shared(mock_sink), mt::fake_shared(stub_allocator)};
};

TEST_F(ClientBuffers, sends_full_buffer_on_allocation)
{
    auto stub_buffer = std::make_shared<mtd::StubBuffer>();
    EXPECT_CALL(mock_allocator, alloc_buffer(Ref(properties)))
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(mock_sink, send_buffer(stream_id, Ref(*stub_buffer), mg::BufferIpcMsgType::full_msg));
    mc::BufferMap map{stream_id, mt::fake_shared(mock_sink), mt::fake_shared(mock_allocator)};
    map.add_buffer(properties);
}

TEST_F(ClientBuffers, access_of_nonexistent_buffer_throws)
{
    auto stub_buffer = std::make_unique<mtd::StubBuffer>();
    EXPECT_THROW({
        auto buffer = map[stub_buffer->id()];
    }, std::logic_error);
}

TEST_F(ClientBuffers, removal_of_nonexistent_buffer_throws)
{
    auto stub_buffer = std::make_unique<mtd::StubBuffer>();
    EXPECT_THROW({
        map.remove_buffer(stub_buffer->id());
    }, std::logic_error);
}

TEST_F(ClientBuffers, can_access_once_added)
{
    map.add_buffer(properties);
    ASSERT_THAT(stub_allocator.map, SizeIs(1));
    ASSERT_THAT(stub_allocator.ids, SizeIs(1));
    auto buffer = map[stub_allocator.ids[0]];
    EXPECT_THAT(buffer, Eq(stub_allocator.map[stub_allocator.ids[0]]));
}

TEST_F(ClientBuffers, sends_update_msg_to_send_buffer)
{
    map.add_buffer(properties);
    ASSERT_THAT(stub_allocator.map, SizeIs(1));
    ASSERT_THAT(stub_allocator.ids, SizeIs(1));
    auto buffer = map[stub_allocator.ids[0]];
    EXPECT_CALL(mock_sink, send_buffer(stream_id, Ref(*buffer), mg::BufferIpcMsgType::update_msg));
    map.send_buffer(stub_allocator.ids[0]);
}

TEST_F(ClientBuffers, sends_no_update_msg_if_buffer_is_not_around)
{
    map.add_buffer(properties);
    ASSERT_THAT(stub_allocator.map, SizeIs(1));
    ASSERT_THAT(stub_allocator.ids, SizeIs(1));
    auto buffer = map[stub_allocator.ids[0]];
    EXPECT_CALL(mock_sink, send_buffer(stream_id, Ref(*buffer), mg::BufferIpcMsgType::update_msg))
        .Times(0);
    map.remove_buffer(stub_allocator.ids[0]);
    map.send_buffer(stub_allocator.ids[0]);
}
