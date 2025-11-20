/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <mir/geometry/point.h>

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

TEST(geometry, point_can_be_converted_from_int_to_float)
{
    using namespace geom;

    Point const i{1, 3};
    PointF const f{i};

    EXPECT_EQ(XF(1.0), f.x);
    EXPECT_EQ(YF(3.0), f.y);
}

TEST(geometry, point_can_be_converted_from_float_to_int)
{
    using namespace geom;

    PointF const f{1.2, 3.0};
    Point const i{f};

    EXPECT_EQ(X(1), i.x);
    EXPECT_EQ(Y(3), i.y);
}
