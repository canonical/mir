/*
 * Copyright © 2013 Canonical Ltd.
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

geom::Point rect_bottom_right(geom::Rectangle const& rect)
{
    return {geom::X{rect.top_left.x.as_int() + rect.size.width.as_int() - 1},
            geom::Y{rect.top_left.y.as_int() + rect.size.height.as_int() - 1}};
}

geom::Rectangle rect_from_points(geom::Point const& tl, geom::Point const& br)
{
    return {tl,
            geom::Size{geom::Width{br.x.as_int() - tl.x.as_int() + 1},
                       geom::Height{br.y.as_int() - tl.y.as_int() + 1}}};
}

}

geom::Rectangles::Rectangles()
{
}

void geom::Rectangles::add(geom::Rectangle const& rect)
{
    rectangles.push_back(rect);
    update_bounding_rectangle();
}

void geom::Rectangles::clear()
{
    rectangles.clear();
    update_bounding_rectangle();
}

geom::Rectangle geom::Rectangles::bounding_rectangle() const
{
    return bounding_rectangle_;
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

void geom::Rectangles::update_bounding_rectangle()
{
    if (rectangles.size() == 0)
    {
        bounding_rectangle_ = geom::Rectangle();
        return;
    }

    geom::Point tl;
    geom::Point br;
    bool points_initialized{false};

    for (auto& rect : rectangles)
    {
        if (rect.size.width.as_int() > 0 &&
            rect.size.height.as_int() > 0)
        {
            geom::Point rtl = rect.top_left;
            geom::Point rbr = rect_bottom_right(rect);

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
    }

    bounding_rectangle_ = rect_from_points(tl, br);
}

std::ostream& geom::operator<<(std::ostream& out, Rectangles const& value)
{
    out << '[';
    for (auto const& rect : value)
        out << rect << ", ";
    out << ']';
    return out;
}
