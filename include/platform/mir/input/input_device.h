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

#ifndef MIR_INPUT_INPUT_DEVICE_H_
#define MIR_INPUT_INPUT_DEVICE_H_

#include "mir/module_deleter.h"
#include "mir/optional_value.h"

#include <memory>

namespace mir
{
namespace dispatch
{
class Dispatchable;
}
namespace input
{
class InputSink;
class InputDeviceInfo;
class EventBuilder;

class PointerSettings;
class TouchpadSettings;
class TouchscreenSettings;

/**
 * Represents an input device.
 */
class InputDevice
{
public:
    InputDevice() = default;
    virtual ~InputDevice() = default;

    /*!
     * Allow the input device to provide its input events to the given InputSink
     */
    virtual void start(InputSink* destination, EventBuilder* builder) = 0;
    /*!
     * Stop the input device from sending input events, to the InputSink.
     */
    virtual void stop() = 0;

    virtual InputDeviceInfo get_device_info() = 0;

    virtual optional_value<PointerSettings> get_pointer_settings() const = 0;
    virtual void apply_settings(PointerSettings const&) = 0;

    virtual optional_value<TouchpadSettings> get_touchpad_settings() const = 0;
    virtual void apply_settings(TouchpadSettings const&) = 0;

    virtual optional_value<TouchscreenSettings> get_touchscreen_settings() const = 0;
    virtual void apply_settings(TouchscreenSettings const&) = 0;
protected:
    InputDevice(InputDevice const&) = delete;
    InputDevice& operator=(InputDevice const&) = delete;
};

}
}

#endif
