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

#ifndef MIR_SHELL_DISPLAY_SURFACE_BOUNDARIES_H_
#define MIR_SHELL_DISPLAY_SURFACE_BOUNDARIES_H_

#include "mir/shell/surface_boundaries.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Display;
}
namespace shell
{

class DisplaySurfaceBoundaries : public SurfaceBoundaries
{
public:
    DisplaySurfaceBoundaries(std::shared_ptr<graphics::Display> const& display);

    void clip_to_screen(geometry::Rectangle& rect);
    void make_fullscreen(geometry::Rectangle& rect);

private:
    geometry::Rectangle get_screen_for(geometry::Rectangle& rect);

    std::shared_ptr<graphics::Display> const display;
};

}
}

#endif /* MIR_SHELL_DISPLAY_SURFACE_BOUNDARIES_H_ */
