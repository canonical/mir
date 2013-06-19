/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_RECTANGLES_INPUT_REGION_H_
#define MIR_INPUT_RECTANGLES_INPUT_REGION_H_

#include "mir/input/input_region.h"
#include "mir/geometry/rectangle.h"

#include <initializer_list>
#include <vector>

namespace mir
{
namespace input
{

class RectanglesInputRegion : public InputRegion
{
public:
    RectanglesInputRegion(std::initializer_list<geometry::Rectangle> const& input_rectangles);
    virtual ~RectanglesInputRegion() noexcept(true) = default;

    bool contains_point(uint32_t x, uint32_t y) const;

protected:
    RectanglesInputRegion(RectanglesInputRegion const&) = delete;
    RectanglesInputRegion& operator=(RectanglesInputRegion const&) = delete;

private:
    std::vector<geometry::Rectangle> input_rectangles;
};

}
}

#endif // MIR_INPUT_RECTANGLES_INPUT_REGION
