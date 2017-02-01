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

#include "src/client/connection_surface_map.h"
#include "src/client/mir_surface.h"
#include "src/client/presentation_chain.h"
#include "src/client/render_surface.h"
#include "mir/test/doubles/mock_mir_buffer_stream.h"
#include "mir/test/doubles/mock_protobuf_server.h"
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mcl = mir::client;
namespace mtd = mir::test::doubles;

namespace
{
void buffer_cb(MirBuffer*, void*)
{
}
}
struct ConnectionResourceMap : testing::Test
{
    std::shared_ptr<MirWaitHandle> wh { std::make_shared<MirWaitHandle>() };
    std::shared_ptr<MirWindow> surface{std::make_shared<MirWindow>("a string", nullptr, mf::SurfaceId{2}, wh)};
    std::shared_ptr<MirBufferStream> stream{ std::make_shared<mtd::MockMirBufferStream>() }; 
    std::shared_ptr<mcl::Buffer> buffer {
        std::make_shared<mcl::Buffer>(buffer_cb, nullptr, 0, nullptr, nullptr, mir_buffer_usage_software) };
    mtd::MockProtobufServer mock_server;
    std::shared_ptr<mcl::PresentationChain> chain{ std::make_shared<mcl::PresentationChain>(
        nullptr, 0, mock_server, nullptr, nullptr) };
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    std::shared_ptr<MirRenderSurface> render_surface { std::make_shared<mcl::RenderSurface>(
        nullptr, nullptr, nullptr, nullptr, mir::geometry::Size{0, 0}) };
#pragma GCC diagnostic pop

    mf::SurfaceId const surface_id{43};
    mf::BufferStreamId const stream_id{43};
    int const buffer_id{43};
    int void_ptr{0};
};

TEST_F(ConnectionResourceMap, maps_surface_when_surface_inserted)
{
    using namespace testing;
    mcl::ConnectionSurfaceMap map;
    map.insert(surface_id, surface);
    EXPECT_EQ(surface, map.surface(surface_id));
}

TEST_F(ConnectionResourceMap, removes_surface_when_surface_removed)
{
    using namespace testing;
    mcl::ConnectionSurfaceMap map;
    map.insert(surface_id, surface);
    map.erase(surface_id);
    EXPECT_FALSE(map.stream(stream_id));
}

TEST_F(ConnectionResourceMap, maps_streams)
{
    using namespace testing;
    mcl::ConnectionSurfaceMap map;
    map.insert(stream_id, stream);
    EXPECT_EQ(stream, map.stream(stream_id));
    map.erase(stream_id);
}

TEST_F(ConnectionResourceMap, holds_chain_reference)
{
    using namespace testing;
    mcl::ConnectionSurfaceMap map;
    auto use_count = chain.use_count();
    map.insert(stream_id, chain);
    EXPECT_THAT(chain.use_count(), Gt(use_count));
    map.erase(stream_id);
    EXPECT_THAT(chain.use_count(), Eq(use_count));
}

TEST_F(ConnectionResourceMap, maps_buffers)
{
    using namespace testing;
    mcl::ConnectionSurfaceMap map;
    EXPECT_THROW({
        map.buffer(buffer_id);
    }, std::runtime_error);

    map.insert(buffer_id, buffer);
    EXPECT_THAT(map.buffer(buffer_id), Eq(buffer));
    map.erase(buffer_id);

    EXPECT_THROW({
        map.buffer(buffer_id);
    }, std::runtime_error);
}

TEST_F(ConnectionResourceMap, can_access_buffers_from_surface)
{
    using namespace testing;
    mcl::ConnectionSurfaceMap map;
    map.insert(buffer_id, buffer);
    map.insert(surface_id, surface);
    EXPECT_EQ(buffer, map.buffer(buffer_id));
}

TEST_F(ConnectionResourceMap, can_insert_retrieve_erase_render_surface)
{
    using namespace testing;
    mcl::ConnectionSurfaceMap map;

    map.insert(&void_ptr, render_surface);
    auto rs = map.render_surface(&void_ptr);
    map.erase(&void_ptr);

    Sequence seq;
    EXPECT_THAT(rs.get(), Eq(render_surface.get()));
    EXPECT_THROW({
        map.render_surface(&void_ptr);
    }, std::runtime_error);
}
