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

#ifndef MIR_GEOMETRY_RECTANGLES_H_
#define MIR_GEOMETRY_RECTANGLES_H_

#include "rectangle.h"

#include <vector>

namespace mir
{
namespace geometry
{

class Rectangles
{
public:
    Rectangles();
    /* We want to keep implicit copy and move methods */

    void add(Rectangle const& rect);
    Rectangle bounding_rectangle() const;

    typedef std::vector<Rectangle>::const_iterator const_iterator;
    typedef std::vector<Rectangle>::size_type size_type;
    const_iterator begin() const;
    const_iterator end() const;
    size_type size() const;

    bool operator==(Rectangles const& rect) const;
    bool operator!=(Rectangles const& rect) const;

private:
    void update_bounding_rectangle();

    std::vector<Rectangle> rectangles;
    Rectangle bounding_rectangle_;
};


std::ostream& operator<<(std::ostream& out, Rectangles const& value);

}
}

#endif /* MIR_GEOMETRY_RECTANGLES_H_ */
