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

TEST(geometry, rectangle_bottom_right)
{
    using namespace testing;
    using namespace geom;

    Rectangle const rect{{0,0}, {1,1}};
    Rectangle const rect_empty{{2,2}, {0,0}};

    EXPECT_EQ(Point(1,1), rect.bottom_right());
    EXPECT_EQ(Point(2,2), rect_empty.bottom_right());
}

TEST(geometry, rectangle_contains_rectangle)
{
    using namespace testing;
    using namespace geom;

    const int left = 3;
    const int top = 3;
    const int width = 3;
    const int height = 3;
    const Rectangle r{{left,top}, {width,height}};

    const int right = left + width;
    const int bottom = top + height;

    // Test for _every_ rectangle contained in r
    for (int y = top; y <= bottom; y++)
    {
        for (int x = left; x <= right; x++)
        {
            const int max_height = bottom - y;
            const int max_width = right - x;
            for (int h = 0; h <= max_height; h++)
            {
                for (int w = 0; w <= max_width; w++)
                {
                    EXPECT_TRUE(r.contains(Rectangle{{x,y}, {w,h}}));
                }
            }

            EXPECT_FALSE(r.contains(Rectangle{{x,y}, {max_width+1,max_height}}));
            EXPECT_FALSE(r.contains(Rectangle{{x,y}, {max_width,max_height+1}}));
        }
    }

    EXPECT_FALSE(r.contains(Rectangle{{2,2}, {5,5}}));
    EXPECT_FALSE(r.contains(Rectangle{{2,2}, {1,1}}));
    EXPECT_FALSE(r.contains(Rectangle{{-4,-3}, {10,10}}));

    EXPECT_FALSE(r.contains(Rectangle{{1,3}, {3,3}}));
    EXPECT_FALSE(r.contains(Rectangle{{3,1}, {3,3}}));
    EXPECT_FALSE(r.contains(Rectangle{{5,3}, {3,3}}));
    EXPECT_FALSE(r.contains(Rectangle{{3,5}, {3,3}}));

    EXPECT_FALSE(r.contains(Rectangle{{9,9}, {1,1}}));
    EXPECT_FALSE(r.contains(Rectangle{{-6,-7}, {3,4}}));
}

TEST(geometry, empty_rectangle_contains_point_only)
{
    using namespace testing;
    using namespace geom;

    const int left = 3;
    const int top = 3;
    const Rectangle r{{left,top}, {0,0}};

    EXPECT_TRUE(r.contains(Rectangle{{left,top}, {0,0}}));

    EXPECT_FALSE(r.contains(Rectangle{{left,top}, {1,0}}));
    EXPECT_FALSE(r.contains(Rectangle{{left,top}, {0,1}}));
    EXPECT_FALSE(r.contains(Rectangle{{left,top}, {1,1}}));
    EXPECT_FALSE(r.contains(Rectangle{{left-1,top}, {3,0}}));
    EXPECT_FALSE(r.contains(Rectangle{{left-1,top}, {0,0}}));
    EXPECT_FALSE(r.contains(Rectangle{{left,top+1}, {0,0}}));
}

TEST(geometry, elongated_empty_rectangle_contains_points_only)
{
    using namespace testing;
    using namespace geom;

    const int left = 3;
    const int top = 3;
    const Rectangle r{{left,top}, {0,3}};

    EXPECT_TRUE(r.contains(Rectangle{{left,top}, {0,0}}));
    EXPECT_TRUE(r.contains(Rectangle{{left,top+1}, {0,0}}));
    EXPECT_TRUE(r.contains(Rectangle{{left,top+2}, {0,0}}));
    EXPECT_TRUE(r.contains(Rectangle{{left,top+3}, {0,0}}));

    EXPECT_TRUE(r.contains(Rectangle{{left,top}, {0,1}}));
    EXPECT_TRUE(r.contains(Rectangle{{left,top+1}, {0,2}}));

    EXPECT_FALSE(r.contains(Rectangle{{left,top+4}, {0,0}}));
    EXPECT_FALSE(r.contains(Rectangle{{left,top}, {1,0}}));
    EXPECT_FALSE(r.contains(Rectangle{{left,top}, {1,1}}));
    EXPECT_FALSE(r.contains(Rectangle{{left-1,top}, {3,0}}));
    EXPECT_FALSE(r.contains(Rectangle{{left-1,top}, {0,0}}));
}

TEST(geometry, rectangle_contains_point)
{
    using namespace testing;
    using namespace geom;

    Rectangle const rect{{0,0}, {1,1}};
    Rectangle const rect_empty{{2,2}, {0,0}};

    EXPECT_TRUE(rect.contains({0,0}));
    EXPECT_FALSE(rect.contains({0,1}));
    EXPECT_FALSE(rect.contains({1,0}));
    EXPECT_FALSE(rect.contains({1,1}));

    EXPECT_FALSE(rect_empty.contains({2,2}));
}

TEST(geometry, rectangle_overlaps)
{
    using namespace testing;
    using namespace geom;

    Rectangle const rect1{{0,0}, {1,1}};
    Rectangle const rect2{{1,1}, {1,1}};
    Rectangle const rect3{{0,0}, {2,2}};
    Rectangle const rect4{{-1,-1}, {2,2}};
    Rectangle const rect_empty{{0,0}, {0,0}};

    EXPECT_FALSE(rect_empty.overlaps(rect_empty));
    EXPECT_FALSE(rect_empty.overlaps(rect1));
    EXPECT_FALSE(rect_empty.overlaps(rect4));

    EXPECT_FALSE(rect1.overlaps(rect2));
    EXPECT_FALSE(rect2.overlaps(rect1));
    EXPECT_FALSE(rect4.overlaps(rect2));
    EXPECT_FALSE(rect2.overlaps(rect4));

    EXPECT_TRUE(rect1.overlaps(rect1));
    EXPECT_TRUE(rect4.overlaps(rect4));

    EXPECT_TRUE(rect3.overlaps(rect1));
    EXPECT_TRUE(rect1.overlaps(rect3));
    EXPECT_TRUE(rect3.overlaps(rect2));
    EXPECT_TRUE(rect2.overlaps(rect3));

    EXPECT_TRUE(rect4.overlaps(rect1));
    EXPECT_TRUE(rect1.overlaps(rect4));
    EXPECT_TRUE(rect4.overlaps(rect3));
    EXPECT_TRUE(rect3.overlaps(rect3));
}
