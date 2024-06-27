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

#include "mir/input/virtual_input_device.h"

#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"
#include "mir/input/touchscreen_settings.h"

#include <atomic>

namespace mi = mir::input;

namespace
{
auto unique_device_id() -> std::string
{
    static std::atomic<int> next_id = 0;
    auto const id = next_id.fetch_add(1);
    return "virt-dev-" + std::to_string(id);
}
}

mi::VirtualInputDevice::VirtualInputDevice(std::string const& name, DeviceCapabilities capabilities)
    : info{name, unique_device_id(), capabilities}
{
}

void mi::VirtualInputDevice::if_started_then(std::function<void(InputSink*, EventBuilder*)> const& fn)
{
    auto const lock = state.lock();
    if (auto const locked = *lock)
    {
        fn(locked->sink, locked->builder);
    }
}

void mi::VirtualInputDevice::set_leds(mir::input::KeyboardLeds) { }

void mi::VirtualInputDevice::start(InputSink* sink, EventBuilder* builder)
{
    *state.lock() = State{sink, builder};
}

void mi::VirtualInputDevice::stop()
{
    *state.lock() = std::nullopt;
}

auto mi::VirtualInputDevice::get_pointer_settings() const -> optional_value<PointerSettings>
{
    return {};
}

auto mi::VirtualInputDevice::get_touchpad_settings() const -> optional_value<TouchpadSettings>
{
    return {};
}
auto mi::VirtualInputDevice::get_touchscreen_settings() const -> optional_value<TouchscreenSettings>
{
    return {};
}
