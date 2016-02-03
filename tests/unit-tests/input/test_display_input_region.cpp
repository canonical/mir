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

#include "src/server/input/display_input_region.h"

#include <tuple>

#include <gtest/gtest.h>

namespace mi = mir::input;
namespace geom = mir::geometry;

namespace
{
geom::Rectangles const rects{
    geom::Rectangle{{0,0}, {800,600}},
    geom::Rectangle{{0,600}, {100,100}},
    geom::Rectangle{{800,0}, {100,100}}
};
}

TEST(DisplayInputRegionTest, returns_correct_bounding_rectangle)
{
    geom::Rectangle const expected_bounding_rect{geom::Point{0,0}, geom::Size{800,600}};

    mi::DisplayInputRegion input_region;
    input_region.set_input_rectangles(rects);

    auto rect = input_region.bounding_rectangle();
    EXPECT_EQ(expected_bounding_rect, rect);
}

TEST(DisplayInputRegionTest, confines_point_to_closest_valid_position)
{
    mi::DisplayInputRegion input_region;
    input_region.set_input_rectangles(rects);

    std::vector<std::tuple<geom::Point,geom::Point>> point_tuples{
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

    for (auto const& t : point_tuples)
    {
        geom::Point confined_point{std::get<0>(t)};
        geom::Point const expected_point{std::get<1>(t)};
        input_region.confine(confined_point);
        EXPECT_EQ(expected_point, confined_point);
    }

}

TEST(DisplayInputRegionTest, returns_empty_bounding_rectangle_when_there_are_no_outputs)
{
    geom::Rectangles const empty_rects{};
    geom::Rectangle const empty_rect{};

    mi::DisplayInputRegion input_region;
    input_region.set_input_rectangles(empty_rects);

    auto const bounding_rect = input_region.bounding_rectangle();
    EXPECT_EQ(empty_rect, bounding_rect);
}
