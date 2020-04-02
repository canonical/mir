/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_GRAPHICS_CURSOR_H_
#define MIR_GRAPHICS_CURSOR_H_

#include "mir/geometry/size.h"
#include "mir/geometry/point.h"

#include <memory>

namespace mir
{
namespace graphics
{
class CursorImage;
class Cursor
{
public:
    virtual void show() = 0;
    virtual void show(CursorImage const& cursor_image) = 0;
    virtual void hide() = 0;

    virtual void move_to(geometry::Point position) = 0;

protected:
    Cursor() = default;
    virtual ~Cursor() = default;
    Cursor(Cursor const&) = delete;
    Cursor& operator=(Cursor const&) = delete;
};
}
}


#endif /* MIR_GRAPHICS_CURSOR_H_ */
