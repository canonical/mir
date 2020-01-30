/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_SHELL_DISPLAY_LAYOUT_H_
#define MIR_SHELL_DISPLAY_LAYOUT_H_

#include "mir/graphics/display_configuration.h"

namespace mir
{
namespace geometry
{
struct Rectangle;
}
namespace shell
{

/**
 * Interface to the layout of the display outputs.
 */
class DisplayLayout
{
public:
    virtual ~DisplayLayout() = default;

    /**
     * Clips a rectangle to the output it is in.
     *
     * @param [in,out] rect the rectangle to clip
     */
    virtual void clip_to_output(geometry::Rectangle& rect) = 0;

    /**
     * Makes a rectangle take up the whole area of the output it is in.
     *
     * @param [in,out] rect the rectangle to make fullscreen
     */
    virtual void size_to_output(geometry::Rectangle& rect) = 0;

    /**
     * Places a rectangle in an particular output if the display is known,
     * otherwise does nothing.
     *
     * @param [in]     id   the id of the output to place the rectangle in
     * @param [in,out] rect the rectangle to place
     * @return true iff the display id is recognised
     */
    virtual bool place_in_output(graphics::DisplayConfigurationOutputId id,
                                 geometry::Rectangle& rect) = 0;

protected:
    DisplayLayout() = default;
    DisplayLayout(DisplayLayout const&) = delete;
    DisplayLayout& operator=(DisplayLayout const&) = delete;
};

}
}

#endif /* MIR_SHELL_DISPLAY_LAYOUT_H_ */
