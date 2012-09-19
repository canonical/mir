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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/geometry/dimensions.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;

TEST(geometry, width)
{
    geom::Width width0 {0};
    geom::Width width42 {42};

    EXPECT_EQ(uint32_t{0}, width0.as_uint32_t());
    EXPECT_EQ(uint32_t{42}, width42.as_uint32_t());

    EXPECT_EQ(width0, width0);
    EXPECT_NE(width0, width42);
    EXPECT_EQ(width42, width42);
    EXPECT_NE(width42, width0);
}

TEST(geometry, height)
{
    geom::Height height0 {0};
    geom::Height height42 {42};

    EXPECT_EQ(uint32_t{0}, height0.as_uint32_t());
    EXPECT_EQ(uint32_t{42}, height42.as_uint32_t());

    EXPECT_EQ(height0, height0);
    EXPECT_NE(height0, height42);
    EXPECT_EQ(height42, height42);
    EXPECT_NE(height42, height0);
}

TEST(geometry, delta_arithmetic)
{
    using namespace geom;
    DeltaX dx1{1};
    DeltaY dy1{1};

    DeltaX x2 = DeltaX(1) + dx1;
    EXPECT_EQ(DeltaX(2), x2);
    EXPECT_EQ(DeltaX(1), x2-dx1);
}

TEST(geometry, coordinates)
{
    using namespace geom;
    X x1{1};
    X x2{2};
    DeltaX dx1{1};

    EXPECT_EQ(X(2), x1 + dx1);
    EXPECT_EQ(X(1), x2 - dx1);

    Y y24{24};
    Y y42{42};
    DeltaY dx18{18};

    EXPECT_EQ(dx18, y42 - y24);
}

TEST(geometry, conversions)
{
    using namespace geom;
    Width w1{1};
    DeltaX dx1{1};

    EXPECT_EQ(w1, dim_cast<Width>(dx1));
    EXPECT_EQ(dx1, dim_cast<DeltaX>(w1));
    EXPECT_NE(dx1, dim_cast<DeltaX>(X()));
}
