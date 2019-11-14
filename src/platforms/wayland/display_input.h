/*
 * Copyright © 2019 Canonical Ltd.
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
 */

#ifndef MIR_PLATFORMS_WAYLAND_DISPLAY_INPUT_H_
#define MIR_PLATFORMS_WAYLAND_DISPLAY_INPUT_H_

#include <mir/fd.h>
#include <mir/events/contact_state.h>
#include <mir/geometry/displacement.h>

#include <xkbcommon/xkbcommon.h>

#include <chrono>
#include <vector>

namespace mir
{
namespace input
{
namespace wayland
{
class KeyboardInput
{
public:
    virtual void key_press(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code) = 0;
    virtual void key_release(std::chrono::nanoseconds event_time, xkb_keysym_t key_sym, int32_t key_code) = 0;

    KeyboardInput() = default;
    virtual ~KeyboardInput() = default;
    KeyboardInput(KeyboardInput const&) = delete;
    KeyboardInput& operator=(KeyboardInput const&) = delete;
};

class PointerInput
{
public:
    virtual void pointer_press(std::chrono::nanoseconds event_time, int button, geometry::Point const& pos, geometry::Displacement scroll) = 0;
    virtual void pointer_release(std::chrono::nanoseconds event_time, int button, geometry::Point const& pos, geometry::Displacement scroll) = 0;
    virtual void pointer_motion(std::chrono::nanoseconds event_time, geometry::Point const& pos, geometry::Displacement scroll) = 0;

    PointerInput() = default;
    virtual ~PointerInput() = default;
    PointerInput(PointerInput const&) = delete;
    PointerInput& operator=(PointerInput const&) = delete;
};

class TouchInput
{
public:
    virtual void touch_event(std::chrono::nanoseconds event_time, std::vector<events::ContactState> const& contacts) = 0;

    TouchInput() = default;
    virtual ~TouchInput() = default;
    TouchInput(TouchInput const&) = delete;
    TouchInput& operator=(TouchInput const&) = delete;
};
}
}

}

#endif  // MIR_PLATFORMS_WAYLAND_DISPLAY_INPUT_H_
