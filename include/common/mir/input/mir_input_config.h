/**
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
#include <functional>

struct MirPointerConfig;
struct MirTouchpadConfig;
struct MirKeyboardConfig;
struct MirTouchscreenConfig;

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

    bool has_touchpad_config() const;
    MirTouchpadConfig& touchpad_config();
    MirTouchpadConfig const& touchpad_config() const;
    void set_touchpad_config(MirTouchpadConfig const& conf);

    bool has_keyboard_config() const;
    MirKeyboardConfig& keyboard_config();
    MirKeyboardConfig const& keyboard_config() const;
    void set_keyboard_config(MirKeyboardConfig const& conf);

    bool has_pointer_config() const;
    MirPointerConfig& pointer_config();
    MirPointerConfig const& pointer_config() const;
    void set_pointer_config(MirPointerConfig const& conf);

    bool has_touchscreen_config() const;
    MirTouchscreenConfig& touchscreen_config();
    MirTouchscreenConfig const& touchscreen_config() const;
    void set_touchscreen_config(MirTouchscreenConfig const& conf);

    bool operator==(MirInputDevice const& rhs) const;
    bool operator!=(MirInputDevice const& rhs) const;
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl;
};

// We use "struct", not "class" for consistency with mirclient/mir_toolkit/client_types.h:395
// (To be nice to downstreams that use clang with it's pointless warnings about this.)
struct MirInputConfig
{
public:
    MirInputConfig();
    MirInputConfig(MirInputConfig && conf);
    MirInputConfig(MirInputConfig const& conf);
    MirInputConfig& operator=(MirInputConfig const& conf);
    ~MirInputConfig();

    void add_device_config(MirInputDevice const& conf);
    MirInputDevice* get_device_config_by_id(MirInputDeviceId id);
    MirInputDevice const* get_device_config_by_id(MirInputDeviceId id) const;
    MirInputDevice& get_device_config_by_index(size_t pos);
    MirInputDevice const& get_device_config_by_index(size_t pos) const;
    void remove_device_by_id(MirInputDeviceId id);
    size_t size() const;

    void for_each(std::function<void(MirInputDevice const&)> const& visitor) const;
    void for_each(std::function<void(MirInputDevice &)> const& visitor);
    bool operator==(MirInputConfig const& rhs) const;
    bool operator!=(MirInputConfig const& rhs) const;
    using value_type = MirInputDevice;
private:
    struct Implementation;
    std::unique_ptr<Implementation> impl;
};

std::ostream& operator<<(std::ostream&, MirInputDevice const&);
std::ostream& operator<<(std::ostream&, MirInputConfig const&);

#endif
