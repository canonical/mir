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

#include "mir/input/input_configuration.h"
#include "mir/input/device_capability.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/input/keyboard_configuration.h"
#include "mir/input/pointer_configuration.h"
#include "mir/input/touchscreen_configuration.h"

#include "mir/optional_value.h"
#include <algorithm>
#include <ostream>

namespace mi = mir::input;

struct mi::InputConfiguration::Implementation
{
    // FIXME use a map instead?
    std::vector<mi::DeviceConfiguration> devices;
};

struct mi::DeviceConfiguration::Implementation
{
    Implementation() = default;
    Implementation(MirInputDeviceId id, DeviceCapabilities caps, std::string const& name, std::string const& unique_id)
        : id{id}, caps{caps}, unique_id{unique_id}, name{name}
    {
    }

    MirInputDeviceId id;
    mi::DeviceCapabilities caps;
    std::string unique_id;
    std::string name;

    mir::optional_value<mi::PointerConfiguration> pointer;
    mir::optional_value<mi::TouchpadConfiguration> touchpad;
    mir::optional_value<mi::KeyboardConfiguration> keyboard;
    mir::optional_value<mi::TouchscreenConfiguration> touchscreen;
    // todo add tablet..
};


mi::DeviceConfiguration::DeviceConfiguration()
    : impl(std::make_unique<Implementation>())
{
}

mi::DeviceConfiguration::DeviceConfiguration(MirInputDeviceId id, DeviceCapabilities caps, std::string const& name, std::string const& unique_id)
    : impl(std::make_unique<Implementation>(id, caps, name, unique_id))
{
}

mi::DeviceConfiguration::DeviceConfiguration(DeviceConfiguration && conf)
    : impl{std::move(conf.impl)}
{
}

mi::DeviceConfiguration::DeviceConfiguration(DeviceConfiguration const& conf)
    : impl(std::make_unique<Implementation>(*conf.impl))
{
}

MirInputDeviceId mi::DeviceConfiguration::id() const
{
    return impl->id;
}

mi::DeviceCapabilities mi::DeviceConfiguration::capabilities() const
{
    return impl->caps;
}

std::string const& mi::DeviceConfiguration::name() const
{
    return impl->name;
}

std::string const& mi::DeviceConfiguration::unique_id() const
{
    return impl->unique_id;
}

mi::DeviceConfiguration& mi::DeviceConfiguration::operator=(DeviceConfiguration const& conf)
{
    impl = std::make_unique<Implementation>(*conf.impl);
    return *this;
}

mi::DeviceConfiguration::~DeviceConfiguration() = default;

bool mi::DeviceConfiguration::has_touchpad_configuration() const
{
    return impl->touchpad.is_set();
}

mi::TouchpadConfiguration& mi::DeviceConfiguration::touchpad_configuration()
{
    return impl->touchpad.value();
}

mi::TouchpadConfiguration const& mi::DeviceConfiguration::touchpad_configuration() const
{
    return impl->touchpad.value();
}

void mi::DeviceConfiguration::set_touchpad_configuration(mi::TouchpadConfiguration const& conf)
{
    impl->touchpad = conf;
}

bool mi::DeviceConfiguration::has_touchscreen_configuration() const
{
    return impl->touchscreen.is_set();
}

mi::TouchscreenConfiguration& mi::DeviceConfiguration::touchscreen_configuration()
{
    return impl->touchscreen.value();
}

mi::TouchscreenConfiguration const& mi::DeviceConfiguration::touchscreen_configuration() const
{
    return impl->touchscreen.value();
}

void mi::DeviceConfiguration::set_touchscreen_configuration(TouchscreenConfiguration const& conf)
{
    impl->touchscreen = conf;
}

bool mi::DeviceConfiguration::has_keyboard_configuration() const
{
    return impl->keyboard.is_set();
}

mi::KeyboardConfiguration& mi::DeviceConfiguration::keyboard_configuration()
{
    return impl->keyboard.value();
}

mi::KeyboardConfiguration const& mi::DeviceConfiguration::keyboard_configuration() const
{
    return impl->keyboard.value();
}

void mi::DeviceConfiguration::set_keyboard_configuration(mi::KeyboardConfiguration const& conf)
{
    impl->keyboard = conf;
}

bool mi::DeviceConfiguration::has_pointer_configuration() const
{
    return impl->pointer.is_set();
}

mi::PointerConfiguration& mi::DeviceConfiguration::pointer_configuration()
{
    return impl->pointer.value();
}

mi::PointerConfiguration const& mi::DeviceConfiguration::pointer_configuration() const
{
    return impl->pointer.value();
}

void mi::DeviceConfiguration::set_pointer_configuration(PointerConfiguration const& conf)
{
    impl->pointer = conf;
}

bool mi::DeviceConfiguration::operator==(DeviceConfiguration const& rhs) const
{
    return impl->id == rhs.impl->id &&
        impl->name == rhs.impl->name &&
        impl->caps == rhs.impl->caps &&
        impl->pointer == rhs.impl->pointer &&
        impl->keyboard == rhs.impl->keyboard &&
        impl->touchpad == rhs.impl->touchpad &&
        impl->touchscreen == rhs.impl->touchscreen;
}

bool mi::DeviceConfiguration::operator!=(DeviceConfiguration const& rhs) const
{
    return !(*this == rhs);
}

mi::InputConfiguration::InputConfiguration()
    : impl(std::make_unique<Implementation>())
{
}

mi::InputConfiguration::InputConfiguration(InputConfiguration && conf)
    : impl(std::move(conf.impl))
{
}

mi::InputConfiguration::InputConfiguration(InputConfiguration const& conf)
    : impl(std::make_unique<Implementation>(*conf.impl))
{
}

mi::InputConfiguration::~InputConfiguration() = default;

mi::InputConfiguration& mi::InputConfiguration::InputConfiguration::operator=(InputConfiguration const& conf)
{
    impl = std::make_unique<Implementation>(*conf.impl);
    return *this;
}

void mi::InputConfiguration::add_device_configuration(DeviceConfiguration const& conf)
{
    impl->devices.push_back(conf);
}

size_t mi::InputConfiguration::size() const
{
    return impl->devices.size();
}

void mi::InputConfiguration::for_each(std::function<void(DeviceConfiguration const&)> const& visitor) const
{
    for (auto const& item : impl->devices)
        visitor(item);
}

void mi::InputConfiguration::for_each(std::function<void(DeviceConfiguration &)> const& visitor)
{
    for (auto& item : impl->devices)
        visitor(item);
}

mi::DeviceConfiguration* mi::InputConfiguration::get_device_configuration_by_id(MirInputDeviceId id)
{
    for (auto& item : impl->devices)
        if (item.id() == id)
            return &item;
    return nullptr;
}

mi::DeviceConfiguration const* mi::InputConfiguration::get_device_configuration_by_id(MirInputDeviceId id) const
{
    for (auto const& item : impl->devices)
        if (item.id() == id)
            return &item;
    return nullptr;
}

mi::DeviceConfiguration& mi::InputConfiguration::get_device_configuration_by_index(size_t pos)
{
    return impl->devices[pos];
}

mi::DeviceConfiguration const& mi::InputConfiguration::get_device_configuration_by_index(size_t pos) const
{
    return impl->devices[pos];
}

void mi::InputConfiguration::remove_device_by_id(MirInputDeviceId id)
{
    impl->devices.erase(
        remove_if(begin(impl->devices), end(impl->devices),
                  [id](auto const & conf){ return conf.id() ==id; }),
        end(impl->devices)
        );
}

bool mi::InputConfiguration::operator==(InputConfiguration const& rhs) const
{
    // FIXME assumes fixed ordering
    return impl->devices == rhs.impl->devices;
}

bool mi::InputConfiguration::operator!=(InputConfiguration const& rhs) const
{
    return !(*this == rhs);
}

std::ostream& mi::operator<<(std::ostream& out, mi::DeviceConfiguration const& rhs)
{
    return out << rhs.id() << ' '<< rhs.name()  << ' ' << rhs.unique_id();
}

std::ostream& mi::operator<<(std::ostream& out, mi::InputConfiguration const& rhs)
{
    out << "InputConfiguration{";
    rhs.for_each(
        [&out](DeviceConfiguration const& conf)
        {
           out << '[' << conf << ']';
        });
    return out << "}";
}
