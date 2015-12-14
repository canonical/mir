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
#include "mir/test/doubles/mock_client_buffer_stream.h"
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mcl = mir::client;
namespace mtd = mir::test::doubles;
using namespace testing;

struct ConnectionResourceMap : testing::Test
{
    std::shared_ptr<MirSurface> surface{std::make_shared<MirSurface>("a string", nullptr, mf::SurfaceId{93})};
    mtd::MockClientBufferStream stream; 
    mf::SurfaceId const surface_id{43};
    mf::BufferStreamId const stream_id{43};
};

TEST_F(ConnectionResourceMap, maps_surface_and_bufferstream_when_surface_inserted)
{
    auto surface_called = false;
    mcl::ConnectionSurfaceMap map;
    map.insert(surface_id, surface);
    map.with_surface_do(surface_id, [&](MirSurface* surf) {
        EXPECT_THAT(surf, Eq(surface.get()));
        surface_called = true;
    });

    EXPECT_TRUE(surface_called);
}

TEST_F(ConnectionResourceMap, removes_surface_from_erase)
{
    mcl::ConnectionSurfaceMap map;
    map.insert(surface_id, surface);
    map.erase(surface_id);

    EXPECT_THROW({
        map.with_stream_do(stream_id, [](mcl::ClientBufferStream*){});
    }, std::runtime_error);
}

TEST_F(ConnectionResourceMap, maps_streams)
{
    auto stream_called = false;
    mcl::ConnectionSurfaceMap map;
    map.insert(stream_id, &stream);
    map.with_stream_do(stream_id, [&](mcl::ClientBufferStream* str) {
        EXPECT_THAT(str, Eq(&stream));
        stream_called = true;
    });
    EXPECT_TRUE(stream_called);
    map.erase(stream_id);
}
