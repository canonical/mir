/*
 * Copyright © Canonical Ltd.
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

#ifndef MIR_COMMON_INPUT_DEVICE_STATE_EVENT_H_
#define MIR_COMMON_INPUT_DEVICE_STATE_EVENT_H_

#include <chrono>
#include <limits>
#include <unordered_map>

#include "mir/events/event.h"
#include "mir/events/input_device_state_event.h"

struct MirInputDeviceStateEvent : MirEvent
{
    MirInputDeviceStateEvent();
    auto clone() const -> MirInputDeviceStateEvent* override;

    MirPointerButtons pointer_buttons() const;
    void set_pointer_buttons(MirPointerButtons buttons);

    float pointer_axis(MirPointerAxis axis) const;
    void set_pointer_axis(MirPointerButtons axis, float value);

    std::chrono::nanoseconds when() const;
    void set_when(std::chrono::nanoseconds const& when);

    MirInputEventModifiers modifiers() const;
    void set_modifiers(MirInputEventModifiers modifiers);

    uint32_t device_count() const;
    MirInputDeviceId device_id(size_t index) const;
    MirPointerButtons device_pointer_buttons(size_t index) const;

    uint32_t device_pressed_keys_for_index(size_t index, size_t pressed_index) const;
    uint32_t device_pressed_keys_count(size_t index) const;

    void set_device_states(std::vector<mir::events::InputDeviceState> const& device_states);
    void set_window_id(int id);
    int window_id() const;

private:
    MirInputDeviceStateEvent(MirInputDeviceStateEvent const&) = default;

    MirPointerButtons pointer_buttons_ = 0;
    float pointer_axis_x = 0.0;
    float pointer_axis_y = 0.0;
    std::chrono::nanoseconds when_ = {};
    MirInputEventModifiers modifiers_ = mir_input_event_modifier_none;
    std::vector<mir::events::InputDeviceState> device_states;
    int window_id_ = 0;
};

#endif /* MIR_COMMON_INPUT_DEVICE_STATE_EVENT_H_*/
