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

#ifndef MIR_SHELL_GRAPHICS_DISPLAY_LAYOUT_H_
#define MIR_SHELL_GRAPHICS_DISPLAY_LAYOUT_H_

#include "mir/shell/display_layout.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Display;
}
namespace shell
{

class GraphicsDisplayLayout : public DisplayLayout
{
public:
    GraphicsDisplayLayout(std::shared_ptr<graphics::Display> const& display);

    void clip_to_output(geometry::Rectangle& rect);
    void size_to_output(geometry::Rectangle& rect);
    void place_in_output(graphics::DisplayConfigurationOutputId output_id,
                         geometry::Rectangle& rect);

private:
    geometry::Rectangle get_output_for(geometry::Rectangle& rect);

    std::shared_ptr<graphics::Display> const display;
};

}
}

#endif /* MIR_SHELL_GRAPHICS_DISPLAY_LAYOUT_H_ */
