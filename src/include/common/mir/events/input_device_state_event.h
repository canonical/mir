/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_COMMON_INPUT_DEVICE_STATE_EVENT_H_
#define MIR_COMMON_INPUT_DEVICE_STATE_EVENT_H_

#include <chrono>
#include <limits>

#include "mir/events/event.h"

struct MirInputDeviceStateEvent : MirEvent
{
    MirInputDeviceStateEvent();

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
    uint32_t const* device_pressed_keys(size_t index) const;
    uint32_t device_pressed_keys_count(size_t index) const;

    static mir::EventUPtr deserialize(std::string const& bytes);
    static std::string serialize(MirEvent const* event);
    MirEvent* clone() const;

    void add_device(MirInputDeviceId id, std::vector<uint32_t> && pressed_keys, MirPointerButtons pointer_buttons);

private:
    std::chrono::nanoseconds when_{0};
    MirPointerButtons pointer_buttons_{0};
    MirInputEventModifiers modifiers_{0};

    float pointer_x{0.0f};
    float pointer_y{0.0f};

    struct DeviceState
    {
        DeviceState(MirInputDeviceId id, std::vector<uint32_t> && pressed_keys, MirPointerButtons buttons)
            : id{id}, pressed_keys{std::move(pressed_keys)}, pointer_buttons{buttons}
        {}
        MirInputDeviceId id;
        std::vector<uint32_t> pressed_keys;
        MirPointerButtons pointer_buttons;
    };
    std::vector<DeviceState> devices;
};

#endif /* MIR_COMMON_INPUT_DEVICE_STATE_EVENT_H_*/
