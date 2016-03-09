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
#include "mir/test/doubles/mock_client_buffer_stream.h"
#include "mir/test/doubles/mock_protobuf_server.h"
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mcl = mir::client;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;
using namespace testing;

namespace
{
void buffer_cb(MirPresentationChain*, MirBuffer*, void*)
{
}
}
struct ConnectionResourceMap : testing::Test
{
    std::shared_ptr<MirWaitHandle> wh { std::make_shared<MirWaitHandle>() };
    std::shared_ptr<MirSurface> surface{std::make_shared<MirSurface>("a string", nullptr, mf::SurfaceId{2}, wh)};
    std::shared_ptr<mcl::ClientBufferStream> stream{ std::make_shared<mtd::MockClientBufferStream>() }; 
    std::shared_ptr<mcl::Buffer> buffer { std::make_shared<mcl::Buffer>(buffer_cb, nullptr, 0, nullptr, nullptr) };
    mtd::MockProtobufServer mock_server;
    std::shared_ptr<mcl::PresentationChain> chain{ std::make_shared<mcl::PresentationChain>(
        nullptr, 0, mock_server, nullptr, nullptr) };

    mf::SurfaceId const surface_id{43};
    mf::BufferStreamId const stream_id{43};
    int const buffer_id{43};
};

TEST_F(ConnectionResourceMap, maps_surface_when_surface_inserted)
{
    using namespace testing;
    auto surface_called = false;
    mcl::ConnectionSurfaceMap map;
    map.insert(surface_id, surface);
    map.with_surface_do(surface_id, [&](MirSurface* surf) {
        EXPECT_THAT(surf, Eq(surface.get()));
        surface_called = true;
    });

    EXPECT_TRUE(surface_called);
}

TEST_F(ConnectionResourceMap, removes_surface_when_surface_removed)
{
    using namespace testing;
    mcl::ConnectionSurfaceMap map;
    map.insert(surface_id, surface);
    map.erase(surface_id);
    EXPECT_THROW({
        map.with_stream_do(stream_id, [](mcl::BufferReceiver*){});
    }, std::runtime_error);
}

TEST_F(ConnectionResourceMap, maps_streams)
{
    using namespace testing;
    auto stream_called = false;
    mcl::ConnectionSurfaceMap map;
    map.insert(stream_id, stream);
    map.with_stream_do(stream_id, [&](mcl::BufferReceiver* str) {
        EXPECT_THAT(str, Eq(stream.get()));
        stream_called = true;
    });
    EXPECT_TRUE(stream_called);
    map.erase(stream_id);
}

TEST_F(ConnectionResourceMap, maps_chains)
{
    using namespace testing;
    auto chain_called = false;
    mcl::ConnectionSurfaceMap map;
    map.insert(stream_id, chain);
    map.with_stream_do(stream_id, [&](mcl::BufferReceiver* str) {
        EXPECT_THAT(str, Eq(chain.get()));
        chain_called = true;
    });
    EXPECT_TRUE(chain_called);
    map.erase(stream_id);
}

TEST_F(ConnectionResourceMap, maps_buffers)
{
    using namespace testing;
    mcl::ConnectionSurfaceMap map;
    EXPECT_FALSE(map.with_buffer_do(buffer_id, [](mcl::Buffer&){}));
    map.insert(buffer_id, buffer);
    EXPECT_TRUE(map.with_buffer_do(buffer_id, [this](mcl::Buffer& b)
    {
        EXPECT_THAT(&b, Eq(buffer.get()));
    }));

    map.erase(buffer_id);
    EXPECT_FALSE(map.with_buffer_do(buffer_id, [](mcl::Buffer&){}));
}
