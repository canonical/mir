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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/geometry/rectangles.h"

#include <gtest/gtest.h>

#include <iterator>
#include <algorithm>

namespace geom = mir::geometry;

TEST(geometry, rectangles_empty)
{
    using namespace geom;

    Rectangles const rectangles;

    EXPECT_EQ(0, std::distance(rectangles.begin(), rectangles.end()));
    EXPECT_EQ(0u, rectangles.size());
}

TEST(geometry, rectangles_not_empty)
{
    using namespace geom;

    std::vector<Rectangle> const rectangles_vec = {
        {Point{1, 2}, Size{Width{10}, Height{11}}},
        {Point{3, 4}, Size{Width{12}, Height{13}}},
        {Point{5, 6}, Size{Width{14}, Height{15}}}
    };

    Rectangles rectangles;

    for (auto const& rect : rectangles_vec)
        rectangles.add(rect);

    EXPECT_EQ(static_cast<ssize_t>(rectangles_vec.size()),
              std::distance(rectangles.begin(), rectangles.end()));
    EXPECT_EQ(rectangles_vec.size(), rectangles.size());

    std::vector<bool> rect_found(rectangles_vec.size(), false);

    for (auto const& rect : rectangles)
    {
        auto iter = std::find(rectangles.begin(), rectangles.end(), rect);
        ASSERT_NE(rectangles.end(), iter);
        auto index = std::distance(rectangles.begin(), iter);

        EXPECT_FALSE(rect_found[index]);
        rect_found[index] = true;
    }

    EXPECT_TRUE(std::all_of(rect_found.begin(), rect_found.end(),
                            [](bool b){return b;}));
}

TEST(geometry, rectangles_clear)
{
    using namespace geom;

    std::vector<Rectangle> const rectangles_vec = {
        {Point{1, 2}, Size{Width{10}, Height{11}}},
        {Point{3, 4}, Size{Width{12}, Height{13}}},
        {Point{5, 6}, Size{Width{14}, Height{15}}}
    };

    Rectangles rectangles;
    Rectangles const rectangles_empty;

    for (auto const& rect : rectangles_vec)
        rectangles.add(rect);

    EXPECT_NE(rectangles_empty, rectangles);

    rectangles.clear();

    EXPECT_EQ(rectangles_empty, rectangles);
}

TEST(geometry, rectangles_bounding_rectangle)
{
    using namespace geom;

    std::vector<Rectangle> const rectangles_vec = {
        {Point{0, 0}, Size{Width{10}, Height{5}}},
        {Point{2, 2}, Size{Width{4}, Height{1}}},
        {Point{1, 1}, Size{Width{10}, Height{5}}},
        {Point{-1, -2}, Size{Width{3}, Height{3}}},
        {Point{-2, -3}, Size{Width{14}, Height{10}}},
        {Point{-2, -3}, Size{Width{14}, Height{10}}}
    };

    std::vector<Rectangle> const expected_bounding_rectangles = {
        {Point{0, 0}, Size{Width{10}, Height{5}}},
        {Point{0, 0}, Size{Width{10}, Height{5}}},
        {Point{0, 0}, Size{Width{11}, Height{6}}},
        {Point{-1, -2}, Size{Width{12}, Height{8}}},
        {Point{-2, -3}, Size{Width{14}, Height{10}}},
        {Point{-2, -3}, Size{Width{14}, Height{10}}}
    };

    Rectangles rectangles;

    EXPECT_EQ(Rectangle(), rectangles.bounding_rectangle());

    for (size_t i = 0; i < rectangles_vec.size(); i++)
    {
        rectangles.add(rectangles_vec[i]);
        EXPECT_EQ(expected_bounding_rectangles[i], rectangles.bounding_rectangle()) << "i=" << i;
    }
}

TEST(geometry, rectangles_equality)
{
    using namespace geom;

    std::vector<Rectangle> const rectangles_vec1 = {
        {Point{0, 0}, Size{Width{10}, Height{5}}},
        {Point{2, 2}, Size{Width{4}, Height{1}}},
        {Point{1, 1}, Size{Width{10}, Height{5}}},
        {Point{-1, -2}, Size{Width{3}, Height{3}}},
        {Point{-2, -3}, Size{Width{14}, Height{10}}},
        {Point{-2, -3}, Size{Width{14}, Height{10}}}
    };

    std::vector<Rectangle> const rectangles_vec2 = {
        {Point{0, 0}, Size{Width{10}, Height{5}}},
        {Point{1, 1}, Size{Width{10}, Height{5}}},
        {Point{-1, -2}, Size{Width{3}, Height{3}}},
        {Point{-2, -3}, Size{Width{14}, Height{10}}}
    };

    std::vector<Rectangle> reverse_rectangles_vec1{rectangles_vec1};
    std::reverse(reverse_rectangles_vec1.begin(), reverse_rectangles_vec1.end());

    Rectangles rectangles1;
    Rectangles rectangles2;
    Rectangles rectangles3;
    Rectangles rectangles_empty;

    for (auto const& rect : rectangles_vec1)
        rectangles1.add(rect);

    for (auto const& rect : rectangles_vec2)
        rectangles2.add(rect);

    for (auto const& rect : reverse_rectangles_vec1)
        rectangles3.add(rect);

    EXPECT_EQ(rectangles1, rectangles3);
    EXPECT_NE(rectangles1, rectangles2);
    EXPECT_NE(rectangles2, rectangles3);
    EXPECT_NE(rectangles1, rectangles_empty);
}

TEST(geometry, rectangles_copy_assign)
{
    using namespace geom;

    std::vector<Rectangle> const rectangles_vec = {
        {Point{0, 0}, Size{Width{10}, Height{5}}},
        {Point{2, 2}, Size{Width{4}, Height{1}}},
        {Point{0, 1}, Size{Width{10}, Height{5}}},
        {Point{-1, -2}, Size{Width{3}, Height{3}}},
        {Point{-5, -3}, Size{Width{14}, Height{10}}}
    };

    Rectangles rectangles1;
    Rectangles rectangles2;

    for (auto const& rect : rectangles_vec)
        rectangles1.add(rect);

    rectangles2 = rectangles1;

    Rectangles const rectangles3{rectangles2};

    EXPECT_EQ(rectangles1, rectangles2);
    EXPECT_EQ(rectangles1, rectangles3);
    EXPECT_EQ(rectangles2, rectangles3);

    EXPECT_EQ(rectangles1.bounding_rectangle(), rectangles2.bounding_rectangle());
    EXPECT_EQ(rectangles1.bounding_rectangle(), rectangles3.bounding_rectangle());
    EXPECT_EQ(rectangles2.bounding_rectangle(), rectangles3.bounding_rectangle());
}

TEST(geometry, rectangles_confine)
{
    using namespace geom;

    std::vector<Rectangle> const rectangles_vec = {
        {geom::Point{0,0}, geom::Size{800,600}},
        {geom::Point{0,600}, geom::Size{100,100}},
        {geom::Point{800,0}, geom::Size{100,100}}
    };

    std::vector<std::tuple<geom::Point,geom::Point>> const point_tuples{
        std::make_tuple(geom::Point{0,0}, geom::Point{0,0}),
        std::make_tuple(geom::Point{900,50}, geom::Point{899,50}),
        std::make_tuple(geom::Point{850,100}, geom::Point{850,99}),
        std::make_tuple(geom::Point{801,100}, geom::Point{801,99}),
        std::make_tuple(geom::Point{800,101}, geom::Point{799,101}),
        std::make_tuple(geom::Point{800,600}, geom::Point{799,599}),
        std::make_tuple(geom::Point{-1,700}, geom::Point{0,699}),
        std::make_tuple(geom::Point{-1,-1}, geom::Point{0,0}),
        std::make_tuple(geom::Point{-1,50}, geom::Point{0,50}),
        std::make_tuple(geom::Point{799,-1}, geom::Point{799,0}),
        std::make_tuple(geom::Point{800,-1}, geom::Point{800,0})
    };

    Rectangles rectangles;

    for (auto const& rect : rectangles_vec)
        rectangles.add(rect);

    for (auto const& t : point_tuples)
    {
        geom::Point confined_point{std::get<0>(t)};
        geom::Point const expected_point{std::get<1>(t)};
        rectangles.confine(confined_point);
        EXPECT_EQ(expected_point, confined_point);
    }
}
