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

#include "mir/geometry/size.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;

TEST(geometry, size)
{
    using namespace geom;
    Size const size2x4{2, 4};

    EXPECT_EQ(Width(2), size2x4.width);
    EXPECT_EQ(Height(4), size2x4.height);

    Size const copy = size2x4;
    EXPECT_EQ(Width(2), copy.width);
    EXPECT_EQ(Height(4), copy.height);
    EXPECT_EQ(size2x4, copy);

    Size const defaultValue;
    EXPECT_EQ(Width(0), defaultValue.width);
    EXPECT_EQ(Height(0), defaultValue.height);
    EXPECT_NE(size2x4, defaultValue);
}

TEST(geometry, size_can_be_converted_from_int_to_float)
{
    using namespace geom;

    Size const i{1, 3};
    SizeF const f{i};

    EXPECT_EQ(WidthF(1.0), f.width);
    EXPECT_EQ(HeightF(3.0), f.height);
}

TEST(geometry, size_can_be_converted_from_float_to_int)
{
    using namespace geom;

    SizeF const f{1.2, 3.0};
    Size const i{f};

    EXPECT_EQ(Width(1), i.width);
    EXPECT_EQ(Height(3), i.height);
}

// Test that non-lossy implicit conversions exist
static_assert(std::convertible_to<geom::Size, geom::SizeD>, "Implicit conversion int->double preserves precision");
static_assert(std::convertible_to<geom::SizeF, geom::SizeD>, "Implicit conversion float->double preserves precision");
// Test that lossy implicit conversions fail
static_assert(!std::convertible_to<geom::Size, geom::SizeF>, "Conversion float->int is lossy");
static_assert(!std::convertible_to<geom::SizeD, geom::SizeF>, "Conversion double->float is lossy");
