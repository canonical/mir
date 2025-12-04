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

#ifndef MIR_INPUT_KEYBOARD_OBSERVER_H_
#define MIR_INPUT_KEYBOARD_OBSERVER_H_

#include <mir/events/input_event.h>
#include <memory>

namespace mir
{
namespace input
{
class Surface;

class KeyboardObserver
{
public:
    KeyboardObserver() = default;
    virtual ~KeyboardObserver() = default;

    /// A keyboard or keyboard resync event has been triggered
    virtual void keyboard_event(std::shared_ptr<MirEvent const> const& event) = 0;
    /// The keyboard is now focused on the given surface (or nothing if surface is null)
    virtual void keyboard_focus_set(std::shared_ptr<Surface> const& surface) = 0;

protected:
    KeyboardObserver(const KeyboardObserver&) = delete;
    KeyboardObserver& operator=(const KeyboardObserver&) = delete;
};

}
}

#endif // MIR_INPUT_KEYBOARD_OBSERVER_H_
