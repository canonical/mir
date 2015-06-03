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
#include <gtest/gtest.h>

namespace mcl = mir::client;

struct ConnectionResourceMap : testing::Test
{
    MirSurface surface{"a string"};
    int const id = 43;
};

TEST_F(ConnectionResourceMap, maps_surface_and_bufferstream_when_surface_inserted)
{
    using namespace testing;
    auto called = false;
    mcl::ConnectionSurfaceMap map;
    map.insert(id, &surface);
    map.with_surface_do(id, [&](MirSurface* surf){
        EXPECT_THAT(surf, Eq(&surface));
        called = true;
    });
    EXPECT_TRUE(called);
}
