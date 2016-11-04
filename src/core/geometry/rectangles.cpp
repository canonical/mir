/*
 * Copyright © 2013-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/geometry/rectangles.h"
#include "mir/geometry/displacement.h"
#include <algorithm>

namespace geom = mir::geometry;

namespace
{

template <typename T>
T clamp(T x, T min, T max)
{
    return std::min(std::max(x, min), max);
}


geom::Rectangle rect_from_points(geom::Point const& tl, geom::Point const& br)
{
    return {tl, as_size(br-tl)};
}

}

geom::Rectangles::Rectangles()
{
}

geom::Rectangles::Rectangles(std::initializer_list<Rectangle> const& rects)
    : rectangles{rects}
{
}

void geom::Rectangles::add(geom::Rectangle const& rect)
{
    rectangles.push_back(rect);
}

void geom::Rectangles::remove(Rectangle const& rect)
{
    auto const i = std::find(rectangles.begin(), rectangles.end(), rect);
    if (i != rectangles.end()) rectangles.erase(i);
}

void geom::Rectangles::clear()
{
    rectangles.clear();
}

void geom::Rectangles::confine(geom::Point& point) const
{
    geom::Point ret_point{point};
    geom::Displacement min_dp{geom::DeltaX{std::numeric_limits<int>::max()},
                              geom::DeltaY{std::numeric_limits<int>::max()}};

    for (auto const& rect : rectangles)
    {
        /*
         * If a screen contains the point then no need to confine it further,
         * otherwise confine the point, keeping the confined position that
         * is closer to the original point.
         */
        if (rect.contains(point))
        {
            ret_point = point;
            break;
        }
        else if (rect.size.width > geom::Width{0} &&
                 rect.size.height > geom::Height{0})
        {
            auto br = rect.bottom_right();
            auto min_x = rect.top_left.x;
            auto max_x = geom::X{br.x.as_int() - 1};
            auto min_y = rect.top_left.y;
            auto max_y = geom::Y{br.y.as_int() - 1};

            geom::Point confined_point{clamp(point.x, min_x, max_x),
                                       clamp(point.y, min_y, max_y)};

            /*
             * Keep the confined point that has the least distance to the
             * original point
             */
            auto dp = confined_point - point;
            if (dp < min_dp)
            {
                ret_point = confined_point;
                min_dp = dp;
            }
        }
    }

    point = ret_point;
}


geom::Rectangle geom::Rectangles::bounding_rectangle() const
{
    if (rectangles.size() == 0)
        return geom::Rectangle();

    geom::Point tl;
    geom::Point br;
    bool points_initialized{false};

    for (auto const& rect : rectangles)
    {
        geom::Point rtl = rect.top_left;
        geom::Point rbr = rect.bottom_right();

        if (!points_initialized)
        {
            tl = rtl;
            br = rbr;
            points_initialized = true;
        }
        else
        {
            tl.x = std::min(rtl.x, tl.x);
            tl.y = std::min(rtl.y, tl.y);
            br.x = std::max(rbr.x, br.x);
            br.y = std::max(rbr.y, br.y);
        }
    }

    return rect_from_points(tl, br);
}

geom::Rectangles::const_iterator geom::Rectangles::begin() const
{
    return rectangles.begin();
}

geom::Rectangles::const_iterator geom::Rectangles::end() const
{
    return rectangles.end();
}

geom::Rectangles::size_type geom::Rectangles::size() const
{
    return rectangles.size();
}

bool geom::Rectangles::operator==(Rectangles const& other) const
{
    if (rectangles.size() != other.rectangles.size())
        return false;

    size_t const size{rectangles.size()};
    std::vector<bool> element_used(size, false);

    for (auto const& rect : rectangles)
    {
        bool found{false};

        for (size_t i = 0; i < size; i++)
        {
            if (!element_used[i] && other.rectangles[i] == rect)
            {
                found = true;
                element_used[i] = true;
                break;
            }
        }

        if (!found)
            return false;
    }

    return true;
}

bool geom::Rectangles::operator!=(Rectangles const& rects) const
{
    return !(*this == rects);
}
