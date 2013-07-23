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

#ifndef MIR_INPUT_DISPLAY_INPUT_REGION_H_
#define MIR_INPUT_DISPLAY_INPUT_REGION_H_

#include "mir/input/input_region.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Display;
}
namespace input
{

class DisplayInputRegion : public InputRegion
{
public:
    DisplayInputRegion(std::shared_ptr<graphics::Display> const& display);

    geometry::Rectangle bounding_rectangle();
    void confine(geometry::Point& point);

private:
    std::shared_ptr<graphics::Display> const display;
};

}
}

#endif /* MIR_INPUT_DISPLAY_INPUT_REGION_H_ */

