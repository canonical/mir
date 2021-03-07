/*
 * Copyright © 2016-2021 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include <miroil/input_device.h>

#include "mir/flags.h"
#include "mir/input/device.h"
#include "mir/input/mir_keyboard_config.h"

miroil::InputDevice::InputDevice(std::shared_ptr<mir::input::Device> const& device)
: device(device)
{
}

miroil::InputDevice::InputDevice(InputDevice const& ) = default;

miroil::InputDevice::~InputDevice() = default;

bool miroil::InputDevice::operator==(InputDevice const& other)
{
    return device == other.device;
}
        
void miroil::InputDevice::apply_keymap(std::string const& layout, std::string const& variant)
{
    MirKeyboardConfig oldConfig;
    
    mir::input::Keymap keymap;
    if (device->keyboard_configuration().is_set()) { // preserve the model and options
        oldConfig = device->keyboard_configuration().value();
        keymap.model = oldConfig.device_keymap().model;
        keymap.options = oldConfig.device_keymap().options;
    }
    keymap.layout  = layout;
    keymap.variant = variant;

    device->apply_keyboard_configuration(std::move(keymap));
}
    

MirInputDeviceId miroil::InputDevice::get_device_id()
{
    return device->id();
}

std::string miroil::InputDevice::get_device_name()
{
    return device->name();
}
    
bool miroil::InputDevice::is_keyboard()
{
    return mir::contains(device->capabilities(), mir::input::DeviceCapability::keyboard);
}
        
bool miroil::InputDevice::is_alpha_numeric()
{
    return mir::contains(device->capabilities(), mir::input::DeviceCapability::alpha_numeric);
}

miroil::InputDevice::InputDevice(InputDevice&& ) = default;

auto miroil::InputDevice::operator=(InputDevice const& src) -> InputDevice& = default;

auto miroil::InputDevice::operator=(InputDevice&& src) -> InputDevice& = default;
