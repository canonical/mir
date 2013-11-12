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
