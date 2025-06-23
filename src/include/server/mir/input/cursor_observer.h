/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_INPUT_CURSOR_OBSERVER_H_
#define MIR_INPUT_CURSOR_OBSERVER_H_

namespace mir
{
namespace input
{

/// An interface for listening to absolute cursor events (without context): For example to update
/// the position of the visible cursor.
class CursorObserver
{
public:
    virtual ~CursorObserver() = default;
    virtual void cursor_moved_to(float abs_x, float abs_y) = 0;
    virtual void pointer_usable() = 0;
    virtual void pointer_unusable() = 0;
};

}
}

#endif // MIR_INPUT_CURSOR_OBSERVER_H_
