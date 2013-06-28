/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir/surfaces/surface_info.h"

#include <gtest/gtest.h>

namespace ms = mir::surfaces;
namespace geom = mir::geometry;

struct SurfaceInfoTest : public testing::Test
{
    void SetUp()
    {
        name = std::string("aa");
        top_left = geom::Point{geom::X{4}, geom::Y{7}};
        size = geom::Size{geom::Width{5}, geom::Height{9}};
    }
    std::string name;
    geom::Point top_left;
    geom::Size size;
};

TEST_F(SurfaceInfoTest, basics)
{ 
    ms::SurfaceDataStorage storage{name, top_left, size};
    EXPECT_EQ(name, storage.name());
    EXPECT_EQ(top_left, storage.top_left());
    EXPECT_EQ(size, storage.size());
}

TEST_F(SurfaceInfoTest, update_position)
{ 
    ms::SurfaceDataStorage storage{name, top_left, size};
    EXPECT_EQ(top_left, storage.top_left());

    atuo new_top_left = geom::Point{geom::X{5}, geom::Y{10}};
    storage.set_top_left(new_to_left);
    EXPECT_EQ(new_top_left, storage.top_left());
}
