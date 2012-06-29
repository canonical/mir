/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_GRAPHICS_DISPLAY_H_
#define MIR_GRAPHICS_DISPLAY_H_

#include "mir/geometry/forward.h"

namespace mir
{

namespace graphics
{
class Texture {};

class Display
{
public:
    virtual geometry::Rectangle view_area() = 0;
    virtual void render(Texture const& texture) = 0;

protected:
    Display() = default;
    ~Display() = default;
private:
    Display(Display const&) = delete;
    Display& operator=(Display const&) = delete;
};
}
}

#endif /* MIR_GRAPHICS_DISPLAY_H_ */
