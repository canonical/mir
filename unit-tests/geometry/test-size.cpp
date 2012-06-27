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


TEST(geometry, width)
{
	geom::Width width0{0};
	geom::Width width42{42};

    EXPECT_EQ(0, width0.as_uint32_t());
    EXPECT_EQ(42, width42.as_uint32_t());

    EXPECT_EQ(width0, width0);
    EXPECT_NE(width0, width42);
    EXPECT_EQ(width42, width42);
    EXPECT_NE(width42, width0);
}

TEST(geometry, height)
{
	geom::Height height0{0};
	geom::Height height42{42};

    EXPECT_EQ(0, height0.as_uint32_t());
    EXPECT_EQ(42, height42.as_uint32_t());

    EXPECT_EQ(height0, height0);
    EXPECT_NE(height0, height42);
    EXPECT_EQ(height42, height42);
    EXPECT_NE(height42, height0);
}
