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

#include "mir/geometry/rectangle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace geom = mir::geometry;

TEST(geometry, rectangle)
{
    using namespace geom;
    Point const x3y9{X(3), Y(9)};
    Size const w2h4{2, 4};
    Rectangle const rect{x3y9, w2h4};

    EXPECT_EQ(x3y9, rect.top_left);
    EXPECT_EQ(w2h4, rect.size);

    Rectangle const copy = rect;
    EXPECT_EQ(x3y9, copy.top_left);
    EXPECT_EQ(w2h4, copy.size);
    EXPECT_EQ(rect, copy);

    Rectangle default_rect;
    EXPECT_EQ(Point(), default_rect.top_left);
    EXPECT_EQ(Size(), default_rect.size);
    EXPECT_NE(rect, default_rect);
}
