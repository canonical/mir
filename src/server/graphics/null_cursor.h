/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Gerry Boland <gerry.boland@canonical.com>
 */

#ifndef MIR_GRAPHICS_NULL_CURSOR_H_
#define MIR_GRAPHICS_NULL_CURSOR_H_

#include "mir/graphics/cursor.h"

namespace mir
{
namespace graphics
{

class NullCursor : public Cursor
{
public:
    void show() override {}
    void show(CursorImage const&) override {}
    void hide() override {}
    void move_to(geometry::Point) override {}
};

}
}

#endif
