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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_REGION_H_
#define MIR_INPUT_INPUT_REGION_H_

namespace mir
{
namespace geometry
{
struct Rectangle;
struct Point;
}
namespace input
{

/**
 * Interface to the region of valid input coordinates.
 */
class InputRegion
{
public:
    virtual ~InputRegion() = default;

    /** The bounding rectangle of the input region */
    virtual geometry::Rectangle bounding_rectangle() = 0;

    /**
     * Confines a point to the input region.
     *
     * If the point is within input region it remains unchanged,
     * otherwise it is replaced by the region point that is closest to
     * it.
     *
     * @param [in,out] point the point to confine
     */
    virtual void confine(geometry::Point& point) = 0;

protected:
    InputRegion() = default;
    InputRegion(InputRegion const&) = delete;
    InputRegion& operator=(InputRegion const&) = delete;
};

}
}

#endif /* MIR_INPUT_INPUT_REGION_H_ */
