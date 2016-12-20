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
#include <iosfwd>

struct MirPointerConfiguration;
struct MirTouchpadConfiguration;
struct MirKeyboardConfiguration;
struct MirTouchscreenConfiguration;

class MirInputDevice
{
public:
    MirInputDevice();
    MirInputDevice(MirInputDeviceId id, mir::input::DeviceCapabilities caps, std::string const& name, std::string const& unique_id);
    MirInputDevice(MirInputDevice && conf);
    MirInputDevice(MirInputDevice const& conf);
    MirInputDevice& operator=(MirInputDevice const& conf);
    ~MirInputDevice();

    MirInputDeviceId id() const;
    mir::input::DeviceCapabilities capabilities() const;
    std::string const& name() const;
    std::string const& unique_id() const;

    bool has_touchpad_configuration() const;
    MirTouchpadConfiguration& touchpad_configuration();
    MirTouchpadConfiguration const& touchpad_configuration() const;
    void set_touchpad_configuration(MirTouchpadConfiguration const& conf);

    bool has_keyboard_configuration() const;
    MirKeyboardConfiguration& keyboard_configuration();
    MirKeyboardConfiguration const& keyboard_configuration() const;
    void set_keyboard_configuration(MirKeyboardConfiguration const& conf);

    bool has_pointer_configuration() const;
    MirPointerConfiguration& pointer_configuration();
    MirPointerConfiguration const& pointer_configuration() const;
    void set_pointer_configuration(MirPointerConfiguration const& conf);

    bool has_touchscreen_configuration() const;
    MirTouchscreenConfiguration& touchscreen_configuration();
    MirTouchscreenConfiguration const& touchscreen_configuration() const;
    void set_touchscreen_configuration(MirTouchscreenConfiguration const& conf);

    bool operator==(MirInputDevice const& rhs) const;
    bool operator!=(MirInputDevice const& rhs) const;
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl;
};

class MirInputConfiguration
{
public:
    MirInputConfiguration();
    MirInputConfiguration(MirInputConfiguration && conf);
    MirInputConfiguration(MirInputConfiguration const& conf);
    MirInputConfiguration& operator=(MirInputConfiguration const& conf);
    ~MirInputConfiguration();

    void add_device_configuration(MirInputDevice const& conf);
    MirInputDevice* get_device_configuration_by_id(MirInputDeviceId id);
    MirInputDevice const* get_device_configuration_by_id(MirInputDeviceId id) const;
    MirInputDevice& get_device_configuration_by_index(size_t pos);
    MirInputDevice const& get_device_configuration_by_index(size_t pos) const;
    void remove_device_by_id(MirInputDeviceId id);
    size_t size() const;

    void for_each(std::function<void(MirInputDevice const&)> const& visitor) const;
    void for_each(std::function<void(MirInputDevice &)> const& visitor);
    bool operator==(MirInputConfiguration const& rhs) const;
    bool operator!=(MirInputConfiguration const& rhs) const;
    using value_type = MirInputDevice;
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl;
};

std::ostream& operator<<(std::ostream&, MirInputDevice const&);
std::ostream& operator<<(std::ostream&, MirInputConfiguration const&);

#endif
