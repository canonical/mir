/*
 * Copyright © 2013-2015 Canonical Ltd.
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
#include <gmock/gmock.h>

#include <iterator>
#include <algorithm>

using namespace mir::geometry;
using namespace testing;

namespace
{
struct TestRectangles : Test
{
    Rectangles rectangles;

    auto contents_of(Rectangles const& rects) ->
        std::vector<Rectangle>
    {
        return {std::begin(rects), std::end(rects)};
    }
};
}

TEST_F(TestRectangles, rectangles_empty)
{
    EXPECT_EQ(0, std::distance(rectangles.begin(), rectangles.end()));
    EXPECT_EQ(0u, rectangles.size());
}

TEST_F(TestRectangles, rectangles_not_empty)
{
    std::vector<Rectangle> const rectangles_vec = {
        {Point{1, 2}, Size{Width{10}, Height{11}}},
        {Point{3, 4}, Size{Width{12}, Height{13}}},
        {Point{5, 6}, Size{Width{14}, Height{15}}}
    };

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

TEST_F(TestRectangles, rectangles_clear)
{
    std::vector<Rectangle> const rectangles_vec = {
        {Point{1, 2}, Size{Width{10}, Height{11}}},
        {Point{3, 4}, Size{Width{12}, Height{13}}},
        {Point{5, 6}, Size{Width{14}, Height{15}}}
    };

    Rectangles const rectangles_empty;

    for (auto const& rect : rectangles_vec)
        rectangles.add(rect);

    EXPECT_NE(rectangles_empty, rectangles);

    rectangles.clear();

    EXPECT_EQ(rectangles_empty, rectangles);
}

TEST_F(TestRectangles, rectangles_bounding_rectangle)
{
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

    EXPECT_EQ(Rectangle(), rectangles.bounding_rectangle());

    for (size_t i = 0; i < rectangles_vec.size(); i++)
    {
        rectangles.add(rectangles_vec[i]);
        EXPECT_EQ(expected_bounding_rectangles[i], rectangles.bounding_rectangle()) << "i=" << i;
    }
}

TEST_F(TestRectangles, rectangles_equality)
{
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

TEST_F(TestRectangles, rectangles_copy_assign)
{
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

TEST_F(TestRectangles, rectangles_confine)
{
    std::vector<Rectangle> const rectangles_vec = {
        {Point{0,0}, Size{800,600}},
        {Point{0,600}, Size{100,100}},
        {Point{800,0}, Size{100,100}}
    };

    std::vector<std::tuple<Point,Point>> const point_tuples{
        std::make_tuple(Point{0,0}, Point{0,0}),
        std::make_tuple(Point{900,50}, Point{899,50}),
        std::make_tuple(Point{850,100}, Point{850,99}),
        std::make_tuple(Point{801,100}, Point{801,99}),
        std::make_tuple(Point{800,101}, Point{799,101}),
        std::make_tuple(Point{800,600}, Point{799,599}),
        std::make_tuple(Point{-1,700}, Point{0,699}),
        std::make_tuple(Point{-1,-1}, Point{0,0}),
        std::make_tuple(Point{-1,50}, Point{0,50}),
        std::make_tuple(Point{799,-1}, Point{799,0}),
        std::make_tuple(Point{800,-1}, Point{800,0})
    };

    for (auto const& rect : rectangles_vec)
        rectangles.add(rect);

    for (auto const& t : point_tuples)
    {
        Point confined_point{std::get<0>(t)};
        Point const expected_point{std::get<1>(t)};
        rectangles.confine(confined_point);
        EXPECT_EQ(expected_point, confined_point);
    }
}

TEST_F(TestRectangles, tracks_add_and_remove)
{
    Rectangle const rect[]{
        {{  0,   0}, {800, 600}},
        {{  0, 600}, {100, 100}},
        {{800,   0}, {100, 100}}};

    rectangles = Rectangles{rect[0], rect[1], rect[2]};

    EXPECT_THAT(contents_of(rectangles), UnorderedElementsAre(rect[0], rect[1], rect[2]));
    EXPECT_THAT(rectangles.bounding_rectangle(), Eq(Rectangle{{0,0}, {900,700}}));

    rectangles.remove(rect[1]);

    EXPECT_THAT(contents_of(rectangles), UnorderedElementsAre(rect[0], rect[2]));
    EXPECT_THAT(rectangles.bounding_rectangle(), Eq(Rectangle{{0,0}, {900,600}}));

    rectangles.add(rect[2]);

    EXPECT_THAT(contents_of(rectangles), UnorderedElementsAre(rect[0], rect[2], rect[2]));
    EXPECT_THAT(rectangles.bounding_rectangle(), Eq(Rectangle{{0,0}, {900,600}}));

    rectangles.add(rect[1]);
    rectangles.remove(rect[2]);

    EXPECT_THAT(contents_of(rectangles), UnorderedElementsAre(rect[0], rect[1], rect[2]));
    EXPECT_THAT(rectangles.bounding_rectangle(), Eq(Rectangle{{0,0}, {900,700}}));
}

TEST_F(TestRectangles, can_add_same_rectangle_many_times)
{
    int const many_times = 10;
    Rectangle const rectangle{{  0,   0}, {800, 600}};

    for (auto i = 0; i != many_times; ++i)
        rectangles.add(rectangle);

    EXPECT_THAT(rectangles.size(), Eq(many_times));
}

TEST_F(TestRectangles, remove_only_removes_one_instance)
{
    int const many_times = 10;
    Rectangle const rectangle{{  0,   0}, {800, 600}};

    for (auto i = 0; i != many_times; ++i)
        rectangles.add(rectangle);

    for (auto i = many_times; i-- != 0;)
    {
        rectangles.remove(rectangle);
        EXPECT_THAT(rectangles.size(), Eq(i));
    }
}
