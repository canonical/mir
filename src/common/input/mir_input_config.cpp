/*
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

#include "mir/input/mir_input_config.h"
#include "mir/input/device_capability.h"
#include "mir/input/mir_touchpad_configuration.h"
#include "mir/input/mir_keyboard_configuration.h"
#include "mir/input/mir_pointer_configuration.h"
#include "mir/input/mir_touchscreen_configuration.h"

#include "mir/optional_value.h"
#include <algorithm>
#include <ostream>

namespace mi = mir::input;

struct MirInputConfig::Implementation
{
    // FIXME use a map instead?
    std::vector<MirInputDevice> devices;
};

struct MirInputDevice::Implementation
{
    Implementation() = default;
    Implementation(MirInputDeviceId id, mi::DeviceCapabilities caps, std::string const& name, std::string const& unique_id)
        : id{id}, caps{caps}, unique_id{unique_id}, name{name}
    {
    }

    MirInputDeviceId id;
    mi::DeviceCapabilities caps;
    std::string unique_id;
    std::string name;

    mir::optional_value<MirPointerConfiguration> pointer;
    mir::optional_value<MirTouchpadConfiguration> touchpad;
    mir::optional_value<MirKeyboardConfiguration> keyboard;
    mir::optional_value<MirTouchscreenConfiguration> touchscreen;
    // todo add tablet..
};


MirInputDevice::MirInputDevice()
    : impl(std::make_unique<Implementation>())
{
}

MirInputDevice::MirInputDevice(MirInputDeviceId id, mi::DeviceCapabilities caps, std::string const& name, std::string const& unique_id)
    : impl(std::make_unique<Implementation>(id, caps, name, unique_id))
{
}

MirInputDevice::MirInputDevice(MirInputDevice && conf)
    : impl{std::move(conf.impl)}
{
}

MirInputDevice::MirInputDevice(MirInputDevice const& conf)
    : impl(std::make_unique<Implementation>(*conf.impl))
{
}

MirInputDeviceId MirInputDevice::id() const
{
    return impl->id;
}

mi::DeviceCapabilities MirInputDevice::capabilities() const
{
    return impl->caps;
}

std::string const& MirInputDevice::name() const
{
    return impl->name;
}

std::string const& MirInputDevice::unique_id() const
{
    return impl->unique_id;
}

MirInputDevice& MirInputDevice::operator=(MirInputDevice const& conf)
{
    impl = std::make_unique<Implementation>(*conf.impl);
    return *this;
}

MirInputDevice::~MirInputDevice() = default;

bool MirInputDevice::has_touchpad_configuration() const
{
    return impl->touchpad.is_set();
}

MirTouchpadConfiguration& MirInputDevice::touchpad_configuration()
{
    return impl->touchpad.value();
}

MirTouchpadConfiguration const& MirInputDevice::touchpad_configuration() const
{
    return impl->touchpad.value();
}

void MirInputDevice::set_touchpad_configuration(MirTouchpadConfiguration const& conf)
{
    impl->touchpad = conf;
}

bool MirInputDevice::has_touchscreen_configuration() const
{
    return impl->touchscreen.is_set();
}

MirTouchscreenConfiguration& MirInputDevice::touchscreen_configuration()
{
    return impl->touchscreen.value();
}

MirTouchscreenConfiguration const& MirInputDevice::touchscreen_configuration() const
{
    return impl->touchscreen.value();
}

void MirInputDevice::set_touchscreen_configuration(MirTouchscreenConfiguration const& conf)
{
    impl->touchscreen = conf;
}

bool MirInputDevice::has_keyboard_configuration() const
{
    return impl->keyboard.is_set();
}

MirKeyboardConfiguration& MirInputDevice::keyboard_configuration()
{
    return impl->keyboard.value();
}

MirKeyboardConfiguration const& MirInputDevice::keyboard_configuration() const
{
    return impl->keyboard.value();
}

void MirInputDevice::set_keyboard_configuration(MirKeyboardConfiguration const& conf)
{
    impl->keyboard = conf;
}

bool MirInputDevice::has_pointer_configuration() const
{
    return impl->pointer.is_set();
}

MirPointerConfiguration& MirInputDevice::pointer_configuration()
{
    return impl->pointer.value();
}

MirPointerConfiguration const& MirInputDevice::pointer_configuration() const
{
    return impl->pointer.value();
}

void MirInputDevice::set_pointer_configuration(MirPointerConfiguration const& conf)
{
    impl->pointer = conf;
}

bool MirInputDevice::operator==(MirInputDevice const& rhs) const
{
    return impl->id == rhs.impl->id &&
        impl->name == rhs.impl->name &&
        impl->caps == rhs.impl->caps &&
        impl->pointer == rhs.impl->pointer &&
        impl->keyboard == rhs.impl->keyboard &&
        impl->touchpad == rhs.impl->touchpad &&
        impl->touchscreen == rhs.impl->touchscreen;
}

bool MirInputDevice::operator!=(MirInputDevice const& rhs) const
{
    return !(*this == rhs);
}

MirInputConfig::MirInputConfig()
    : impl(std::make_unique<Implementation>())
{
}

MirInputConfig::MirInputConfig(MirInputConfig && conf)
    : impl(std::move(conf.impl))
{
}

MirInputConfig::MirInputConfig(MirInputConfig const& conf)
    : impl(std::make_unique<Implementation>(*conf.impl))
{
}

MirInputConfig::~MirInputConfig() = default;

MirInputConfig& MirInputConfig::MirInputConfig::operator=(MirInputConfig const& conf)
{
    impl = std::make_unique<Implementation>(*conf.impl);
    return *this;
}

void MirInputConfig::add_device_configuration(MirInputDevice const& conf)
{
    impl->devices.push_back(conf);
}

size_t MirInputConfig::size() const
{
    return impl->devices.size();
}

void MirInputConfig::for_each(std::function<void(MirInputDevice const&)> const& visitor) const
{
    for (auto const& item : impl->devices)
        visitor(item);
}

void MirInputConfig::for_each(std::function<void(MirInputDevice &)> const& visitor)
{
    for (auto& item : impl->devices)
        visitor(item);
}

MirInputDevice* MirInputConfig::get_device_configuration_by_id(MirInputDeviceId id)
{
    for (auto& item : impl->devices)
        if (item.id() == id)
            return &item;
    return nullptr;
}

MirInputDevice const* MirInputConfig::get_device_configuration_by_id(MirInputDeviceId id) const
{
    for (auto const& item : impl->devices)
        if (item.id() == id)
            return &item;
    return nullptr;
}

MirInputDevice& MirInputConfig::get_device_configuration_by_index(size_t pos)
{
    return impl->devices[pos];
}

MirInputDevice const& MirInputConfig::get_device_configuration_by_index(size_t pos) const
{
    return impl->devices[pos];
}

void MirInputConfig::remove_device_by_id(MirInputDeviceId id)
{
    impl->devices.erase(
        remove_if(begin(impl->devices), end(impl->devices),
                  [id](auto const & conf){ return conf.id() ==id; }),
        end(impl->devices)
        );
}

bool MirInputConfig::operator==(MirInputConfig const& rhs) const
{
    // FIXME assumes fixed ordering
    return impl->devices == rhs.impl->devices;
}

bool MirInputConfig::operator!=(MirInputConfig const& rhs) const
{
    return !(*this == rhs);
}

std::ostream& operator<<(std::ostream& out, MirInputDevice const& rhs)
{
    return out << rhs.id() << ' '<< rhs.name()  << ' ' << rhs.unique_id();
}

std::ostream& operator<<(std::ostream& out, MirInputConfig const& rhs)
{
    out << "MirInputConfig{";
    rhs.for_each(
        [&out](MirInputDevice const& conf)
        {
           out << '[' << conf << ']';
        });
    return out << "}";
}
