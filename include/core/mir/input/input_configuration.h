/**
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_INPUT_CONFIGURATION_H
#define MIR_INPUT_INPUT_CONFIGURATION_H

#include "mir_toolkit/mir_input_device_types.h"
#include "mir/input/device_capability.h"

#include <memory>
#include <vector>
#include <string>

namespace mir
{
namespace input
{
struct PointerConfiguration;
struct TouchpadConfiguration;
struct KeyboardConfiguration;
struct TouchscreenConfiguration;

class DeviceConfiguration
{
public:
    DeviceConfiguration();
    DeviceConfiguration(MirInputDeviceId id, DeviceCapabilities caps, std::string const& name, std::string const& unique_id);
    DeviceConfiguration(DeviceConfiguration && conf);
    DeviceConfiguration(DeviceConfiguration const& conf);
    DeviceConfiguration& operator=(DeviceConfiguration const& conf);
    ~DeviceConfiguration();

    MirInputDeviceId id() const;
    DeviceCapabilities capabilities() const;
    std::string const& name() const;
    std::string const& unique_id() const;

    bool has_touchpad_configuration() const;
    TouchpadConfiguration& touchpad_configuration();
    TouchpadConfiguration const& touchpad_configuration() const;
    void set_touchpad_configuration(TouchpadConfiguration const& conf);

    bool has_keyboard_configuration() const;
    KeyboardConfiguration& keyboard_configuration();
    KeyboardConfiguration const& keyboard_configuration() const;
    void set_keyboard_configuration(KeyboardConfiguration const& conf);

    bool has_pointer_configuration() const;
    PointerConfiguration& pointer_configuration();
    PointerConfiguration const& pointer_configuration() const;
    void set_pointer_configuration(PointerConfiguration const& conf);

    bool has_touchscreen_configuration() const;
    TouchscreenConfiguration& touchscreen_configuration();
    TouchscreenConfiguration const& touchscreen_configuration() const;
    void set_touchscreen_configuration(TouchscreenConfiguration const& conf);
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl;
};

class InputConfiguration
{
public:
    InputConfiguration();
    InputConfiguration(InputConfiguration && conf);
    InputConfiguration(InputConfiguration const& conf);
    InputConfiguration& operator=(InputConfiguration const& conf);
    ~InputConfiguration();

    void add_device_configuration(DeviceConfiguration const& conf);
    DeviceConfiguration* get_device_configuration_by_id(MirInputDeviceId id);
    DeviceConfiguration const* get_device_configuration_by_id(MirInputDeviceId id) const;
    DeviceConfiguration& get_device_configuration_by_index(size_t pos);
    DeviceConfiguration const& get_device_configuration_by_index(size_t pos) const;
    size_t size() const;

    void for_each(std::function<void(DeviceConfiguration const&)> const& visitor) const;
    void for_each(std::function<void(DeviceConfiguration &)> const& visitor);
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl;
};

}
}

#endif
