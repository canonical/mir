/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_CURSOR_LISTENER_H_
#define MIR_INPUT_CURSOR_LISTENER_H_

namespace mir
{
namespace input
{

/// An interface for listening to absolute cursor events (without context): For example to update
/// the position of the visible cursor.
class CursorListener
{
public:
    virtual ~CursorListener() = default;

    virtual void cursor_moved_to(float abs_x, float abs_y) = 0;

protected:
    CursorListener() = default;
    CursorListener(CursorListener const&) = delete;
    CursorListener& operator=(CursorListener const&) = delete;
};

}
}

#endif // MIR_INPUT_CURSOR_LISTENER_H_
