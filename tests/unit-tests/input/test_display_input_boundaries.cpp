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

#include "mir/input/display_input_boundaries.h"

#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/null_display_buffer.h"

#include <vector>
#include <tuple>

#include <gtest/gtest.h>

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace mtd = mir::test::doubles;
namespace geom = mir::geometry;

namespace
{

class StubDisplayBuffer : public mtd::NullDisplayBuffer
{
public:
    StubDisplayBuffer(geom::Rectangle const& view_area_) : view_area_(view_area_) {}
    StubDisplayBuffer(StubDisplayBuffer const& s) : view_area_(s.view_area_) {}
    geom::Rectangle view_area() const override { return view_area_; }

private:
    geom::Rectangle view_area_;
};

class StubDisplay : public mtd::NullDisplay
{
public:
    StubDisplay()
        : display_buffers{
              StubDisplayBuffer{geom::Rectangle{geom::Point{0,0}, geom::Size{800,600}}},
              StubDisplayBuffer{geom::Rectangle{geom::Point{0,600}, geom::Size{100,100}}},
              StubDisplayBuffer{geom::Rectangle{geom::Point{800,0}, geom::Size{100,100}}}}
    {

    }

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        for (auto& db : display_buffers)
            f(db);
    }

private:
    std::vector<StubDisplayBuffer> display_buffers;
};

}

TEST(DisplayInputBoundariesTest, returns_correct_bounding_rectangle)
{
    geom::Rectangle const expected_bounding_rect{geom::Point{0,0}, geom::Size{900,700}};
    auto stub_display = std::make_shared<StubDisplay>();

    mi::DisplayInputBoundaries input_boundaries{stub_display};

    auto rect = input_boundaries.bounding_rectangle();
    EXPECT_EQ(expected_bounding_rect, rect);
}

TEST(DisplayInputBoundariesTest, confines_point_to_closest_valid_position)
{
    auto stub_display = std::make_shared<StubDisplay>();

    mi::DisplayInputBoundaries input_boundaries{stub_display};

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
        input_boundaries.confine_point(confined_point);
        EXPECT_EQ(expected_point, confined_point);
    }

}
