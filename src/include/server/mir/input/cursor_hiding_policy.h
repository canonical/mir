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

#ifndef MIR_INPUT_CURSOR_HIDING_POLICY_H_
#define MIR_INPUT_CURSOR_HIDING_POLICY_H_

#include <chrono>
#include <optional>

namespace mir
{
namespace input
{
/// Decides when the cursor is hidden while a pointing device is attached.
///
/// All hide triggers are disabled by default. A hidden cursor is shown again by
/// a pointer button press, a scroll, or pointer motion beyond the reveal threshold.
class CursorHidingPolicy
{
public:
    CursorHidingPolicy() = default;
    virtual ~CursorHidingPolicy() = default;

    /// Hide the cursor on touch-down
    virtual void hide_on_touch(bool enabled) = 0;

    /// Hide the cursor on key press
    virtual void hide_on_keyboard(bool enabled) = 0;

    /// Hide the cursor after \p timeout without pointer events, or never if \p timeout is empty
    virtual void hide_after_inactivity(std::optional<std::chrono::milliseconds> timeout) = 0;

    /// Pointer motion (in logical pixels) needed to show a hidden cursor
    virtual void reveal_threshold(float distance) = 0;

private:
    CursorHidingPolicy(CursorHidingPolicy const&) = delete;
    CursorHidingPolicy& operator=(CursorHidingPolicy const&) = delete;
};
}
}

#endif // MIR_INPUT_CURSOR_HIDING_POLICY_H_
