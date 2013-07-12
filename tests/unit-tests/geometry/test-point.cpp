/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/geometry/point.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;

TEST(geometry, point)
{
    using namespace geom;
    Point const pointx2y4{X(2), Y(4)};

    EXPECT_EQ(X(2), pointx2y4.x);
    EXPECT_EQ(Y(4), pointx2y4.y);

    Point const copy = pointx2y4;
    EXPECT_EQ(X(2), copy.x);
    EXPECT_EQ(Y(4), copy.y);
    EXPECT_EQ(pointx2y4, copy);

    Point defaultValue;
    EXPECT_EQ(X(0), defaultValue.x);
    EXPECT_EQ(Y(0), defaultValue.y);
    EXPECT_NE(pointx2y4, defaultValue);
}

TEST(geometry, point_is_usable)
{
    using namespace geom;

    int x = 0;
    int y = 1;

    auto p = Point{x, y};

    EXPECT_EQ(X(x), p.x);
    EXPECT_EQ(Y(y), p.y);
}
