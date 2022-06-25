/*
 * Copyright © 2013-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_GEOMETRY_RECTANGLES_H_
#define MIR_GEOMETRY_RECTANGLES_H_

#include "mir/geometry/point.h"
#include "mir/geometry/rectangle.h"

#include <vector>
#include <initializer_list>

namespace mir
{
namespace geometry
{

/// A collection of rectangles (with possible duplicates).
class Rectangles
{
public:
    Rectangles();
    Rectangles(std::initializer_list<generic::Rectangle<int>> const& rects);
    /* We want to keep implicit copy and move methods */

    void add(generic::Rectangle<int> const& rect);
    /// removes at most one matching rectangle
    void remove(generic::Rectangle<int> const& rect);
    void clear();
    generic::Rectangle<int> bounding_rectangle() const;
    void confine(Point& point) const;

    typedef std::vector<generic::Rectangle<int>>::const_iterator const_iterator;
    typedef std::vector<generic::Rectangle<int>>::size_type size_type;
    const_iterator begin() const;
    const_iterator end() const;
    size_type size() const;

    bool operator==(Rectangles const& rect) const;
    bool operator!=(Rectangles const& rect) const;

private:
    std::vector<generic::Rectangle<int>> rectangles;
};


std::ostream& operator<<(std::ostream& out, Rectangles const& value);

}
}

#endif /* MIR_GEOMETRY_RECTANGLES_H_ */
