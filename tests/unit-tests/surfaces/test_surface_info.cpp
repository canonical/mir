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

#include "mir/surfaces/surface_data_storage.h"

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
    geom::Rectangle rect{top_left, size};
    ms::SurfaceDataStorage storage{name, top_left, size};
    EXPECT_EQ(name, storage.name());
    EXPECT_EQ(rect, storage.size_and_position());
}

TEST_F(SurfaceInfoTest, update_position)
{ 
    geom::Rectangle rect1{top_left, size};
    ms::SurfaceDataStorage storage{name, top_left, size};
    EXPECT_EQ(rect1, storage.size_and_position());

    auto new_top_left = geom::Point{geom::X{5}, geom::Y{10}};
    geom::Rectangle rect2{new_top_left, size};
    storage.move_to(new_top_left);
    EXPECT_EQ(rect2, storage.size_and_position());
}
