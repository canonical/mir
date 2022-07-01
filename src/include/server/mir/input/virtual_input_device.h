/*
 * Copyright © 2022 Canonical Ltd.
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

#ifndef MIR_INPUT_VIRTUAL_INPUT_DEVICE_H_
#define MIR_INPUT_VIRTUAL_INPUT_DEVICE_H_

#include "mir/input/input_device.h"
#include "mir/input/input_device_info.h"

#include <functional>

namespace mir
{
namespace input
{

/// A source of input events not backed by a physical device
class VirtualInputDevice : public InputDevice
{
public:
    VirtualInputDevice(std::string const& name, DeviceCapabilities capabilities);
    void if_started_then(std::function<void(InputSink*, EventBuilder*)> const& fn);

private:
    void start(InputSink* new_sink, EventBuilder* new_builder) override;
    void stop() override;
    auto get_device_info() -> InputDeviceInfo override { return info; }
    auto get_pointer_settings() const -> optional_value<PointerSettings> override;
    void apply_settings(PointerSettings const&) override {};
    auto get_touchpad_settings() const -> optional_value<TouchpadSettings> override;
    void apply_settings(TouchpadSettings const&) override {};
    auto get_touchscreen_settings() const -> optional_value<TouchscreenSettings> override;
    void apply_settings(TouchscreenSettings const&) override {};

    InputDeviceInfo const info;

    std::mutex mutex;
    InputSink* sink{nullptr};
    EventBuilder* builder{nullptr};
};

}
}

#endif // MIR_INPUT_VIRTUAL_INPUT_DEVICE_H_
