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

struct ClientBuffers : public Test
{
    std::shared_ptr<mtd::MockEventSink> mock_sink = std::make_shared<testing::NiceMock<mtd::MockEventSink>>();
    mg::BufferProperties properties{geom::Size{42,43}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware};
    mtd::StubBuffer stub_buffer{properties};
    mc::BufferMap map{mock_sink};
};

TEST_F(ClientBuffers, sends_full_buffer_on_allocation)
{
    EXPECT_CALL(*mock_sink, add_buffer(Ref(stub_buffer)));
    mc::BufferMap map{mock_sink};
    EXPECT_THAT(map.add_buffer(mt::fake_shared(stub_buffer)), Eq(stub_buffer.id()));
}

TEST_F(ClientBuffers, access_of_nonexistent_buffer_throws)
{
    EXPECT_THROW({
        auto buffer = map[stub_buffer.id()];
    }, std::logic_error);
}

TEST_F(ClientBuffers, removal_of_nonexistent_buffer_throws)
{
    EXPECT_THROW({
        map.remove_buffer(stub_buffer.id());
    }, std::logic_error);
}

TEST_F(ClientBuffers, can_access_once_added)
{
    auto id = map.add_buffer(mt::fake_shared(stub_buffer));
    EXPECT_THAT(map[id].get(), Eq(&stub_buffer));
}

TEST_F(ClientBuffers, sends_update_msg_to_send_buffer)
{
    auto id = map.add_buffer(mt::fake_shared(stub_buffer));
    auto buffer = map[id];
    EXPECT_CALL(*mock_sink, update_buffer(Ref(*buffer)));
    map.send_buffer(id);
}

TEST_F(ClientBuffers, sends_no_update_msg_if_buffer_is_not_around)
{
    auto id = map.add_buffer(mt::fake_shared(stub_buffer));
    auto buffer = map[id];

    EXPECT_CALL(*mock_sink, remove_buffer(Ref(*buffer)));
    map.remove_buffer(id);
    map.send_buffer(id);
}

TEST_F(ClientBuffers, can_remove_buffer_from_send_callback)
{
    auto id = map.add_buffer(mt::fake_shared(stub_buffer));
    ON_CALL(*mock_sink, update_buffer(_))
        .WillByDefault(Invoke(
        [&] (mg::Buffer& buffer)
        {
            map.remove_buffer(buffer.id());
        }));

    map.send_buffer(id);
}

TEST_F(ClientBuffers, ignores_unknown_receive)
{
    EXPECT_CALL(*mock_sink, add_buffer(_))
        .Times(1);
    auto id = map.add_buffer(mt::fake_shared(stub_buffer));
    map.remove_buffer(id);
    map.send_buffer(id);
}

TEST_F(ClientBuffers, sends_error_buffer_when_alloc_fails)
{
    std::string error_msg = "a reason";
    EXPECT_CALL(*mock_sink, add_buffer(_))
        .WillOnce(Throw(std::runtime_error(error_msg)));
    EXPECT_CALL(*mock_sink, error_buffer(stub_buffer.size(), stub_buffer.pixel_format(), StrEq(error_msg)));
    mc::BufferMap map{mock_sink};
    EXPECT_THROW({
        map.add_buffer(mt::fake_shared(stub_buffer));
    }, std::runtime_error);
}
