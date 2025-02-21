/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#include "is_occluded.h"

#include "mir/geometry/rectangle.h"

#include <limits>
#include <algorithm>
#include <ranges>

namespace geom = mir::geometry;

auto miral::is_occluded(
    mir::geometry::Rectangle test_rectangle,
    std::vector<mir::geometry::Rectangle> const& occluding_rectangles,
    mir::geometry::Size min_visible_size) -> bool
{
    // Worst case scenario: you try taking a hole out of the middle of
    // a rect, results in 4 rectangles. Every other case is a special
    // case of this one where the width or height of the resulting
    // rects are < 0.
    auto const subtract_rectangles = [](geom::Rectangle rect,
                                        geom::Rectangle hole) -> std::vector<geom::Rectangle>
    {
        if (!rect.overlaps(hole))
            return {rect};

        auto const rect_a =
            geom::Rectangle{rect.top_left, geom::Size{rect.size.width, (hole.top() - rect.top()).as_int()}};

        auto const positive_or_inf = [](int val)
        {
            return val >= 0 ? val : std::numeric_limits<int>::max();
        };

        auto const rect_b = geom::Rectangle{
            geom::Point{rect.left(), rect.top()},
            geom::Size{
                (hole.left() - rect.left()).as_int(),
                std::min({
                    hole.size.height.as_int(),
                    rect.size.height.as_int(),
                    positive_or_inf((hole.bottom() - rect.top()).as_int()),
                    positive_or_inf((hole.top() - rect.bottom()).as_int()),
                })}};

        auto const rect_c = geom::Rectangle{
            geom::Point{hole.right(), rect.top()},
            geom::Size{
                (rect.right() - hole.right()).as_int(),
                std::min({
                    hole.size.height.as_int(),
                    rect.size.height.as_int(),
                    positive_or_inf((hole.bottom() - rect.top()).as_int()),
                    positive_or_inf((hole.top() - rect.bottom()).as_int()),
                })}};

        auto const rect_d = geom::Rectangle{
            geom::Point{rect.left(), hole.bottom()},
            geom::Size{rect.size.width, (rect.bottom() - hole.bottom()).as_int()}};

        auto const valid_rect = [](auto const rect)
        {
            return rect.size.width.as_int() > 0 && rect.size.height.as_int() > 0;
        };
        // Funny, you can't have valid_rects be const and use `.cbegin`
        auto valid_rects = std::vector{rect_a, rect_b, rect_c, rect_d} | std::ranges::views::filter(valid_rect);

        return std::vector(valid_rects.cbegin(), valid_rects.cend());
    };

    auto const get_visible_rects = [subtract_rectangles](
                                       geom::Rectangle initial_test_rect,
                                       std::vector<geom::Rectangle> const& window_rects) -> std::vector<geom::Rectangle>
    {
        if (window_rects.empty())
            return {initial_test_rect};

        // Starting with rectangle we're trying to place, repeatedly
        // subtract all visible rectangles from it, accumulating
        // subtractions as we go.
        //
        // In the end, if no rectangles remain, then the window is
        // fully occluded. Otherwise, whatever rectangles remain are
        // the visible part.
        std::vector test_rects{initial_test_rect};
        for (auto const& window_rect : window_rects)
        {
            std::vector<geom::Rectangle> iter_unoccluded_areas;
            for (auto const& test_rect : test_rects)
            {
                auto const unoccluded_areas = subtract_rectangles(test_rect, window_rect);
                iter_unoccluded_areas.insert(
                    iter_unoccluded_areas.end(), unoccluded_areas.begin(), unoccluded_areas.end());
            }

            test_rects = std::move(iter_unoccluded_areas);

            if (test_rects.empty())
                return {};
        }

        return test_rects;
    };

    auto const visible_area_large_enough =
        [](std::vector<geom::Rectangle> const& visible_rects, geom::Size min_visible_size)
    {
        auto const min_visible_width = min_visible_size.width;
        auto const min_visible_height = min_visible_size.height;

        return std::ranges::any_of(
            visible_rects,
            [min_visible_width, min_visible_height](auto const& rect)
            {
                return rect.size.width >= min_visible_width && rect.size.height >= min_visible_height;
            });
    };

    auto visible_areas = get_visible_rects(test_rectangle, occluding_rectangles);
    return visible_areas.empty() || !visible_area_large_enough(visible_areas, min_visible_size);
}
