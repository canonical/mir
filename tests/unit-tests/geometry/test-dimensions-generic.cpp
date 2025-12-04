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

#include <mir/geometry/dimensions.h>

#include "boost/throw_exception.hpp"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace mir::geometry::generic;

TEST(geometry, generic_casting_between_same_tag)
{
    Width<int> const width_i {42};
    Width<float> const width_f{width_i};

    EXPECT_EQ(width_i, static_cast<Width<int>>(width_f));
}

TEST(geometry, generic_width)
{
    Width<int> width0 {0};
    Width<int> width42 {42};

    EXPECT_EQ(uint32_t{0}, width0.as_value());
    EXPECT_EQ(uint32_t{42}, width42.as_value());

    EXPECT_EQ(width0, width0);
    EXPECT_NE(width0, width42);
    EXPECT_EQ(width42, width42);
    EXPECT_NE(width42, width0);
}

TEST(geometry, generic_height)
{
    Height<int> height0 {0};
    Height<int> height42 {42};

    EXPECT_EQ(uint32_t{0}, height0.as_value());
    EXPECT_EQ(uint32_t{42}, height42.as_value());

    EXPECT_EQ(height0, height0);
    EXPECT_NE(height0, height42);
    EXPECT_EQ(height42, height42);
    EXPECT_NE(height42, height0);
}

TEST(geometry, generic_delta_arithmetic)
{
    DeltaX<int> dx1{1};

    DeltaX<int> x2 = DeltaX<int>(1) + dx1;
    EXPECT_EQ(DeltaX<int>(2), x2);
    EXPECT_EQ(DeltaX<int>(1), x2-dx1);
}

TEST(geometry, generic_coordinates)
{
    X<int> x1{1};
    X<int> x2{2};
    DeltaX<int> dx1{1};

    EXPECT_EQ(X<int>(2), x1 + dx1);
    EXPECT_EQ(X<int>(1), x2 - dx1);

    Y<int> y24{24};
    Y<int> y42{42};
    DeltaY<int> dx18{18};

    EXPECT_EQ(dx18, y42 - y24);
}

TEST(geometry, generic_signed_dimensions)
{
    X<int> const x0{0};
    X<int> const x2{2};
    X<int> const xn5{-5};
    Y<int> const y0{0};
    Y<int> const y3{3};
    Y<int> const yn6{-6};
    Y<int> const yn7{-7};

    // Compare against zero to catch regressions of signed->unsigned that
    // wouldn't be caught using as_*int()...
    EXPECT_GT(x0, xn5);
    EXPECT_GT(y0, yn7);

    EXPECT_LT(xn5, x0);
    EXPECT_LT(xn5, x2);
    EXPECT_LT(yn7, yn6);
    EXPECT_LT(yn7, y0);
    EXPECT_LT(yn7, y3);

    EXPECT_LE(xn5, x0);
    EXPECT_LE(xn5, x2);
    EXPECT_LE(yn7, yn6);
    EXPECT_LE(yn7, y0);
    EXPECT_LE(yn7, y3);
    EXPECT_LE(yn7, yn7);

    EXPECT_GT(x0, xn5);
    EXPECT_GT(x2, xn5);
    EXPECT_GT(yn6, yn7);
    EXPECT_GT(y0, yn7);
    EXPECT_GT(y3, yn7);

    EXPECT_GE(x0, xn5);
    EXPECT_GE(x2, xn5);
    EXPECT_GE(yn6, yn7);
    EXPECT_GE(y0, yn7);
    EXPECT_GE(y3, yn7);
    EXPECT_GE(yn7, yn7);
}
