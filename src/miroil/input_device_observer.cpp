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

#include "miroil/input_device_observer.h"
#include "mir/flags.h"
#include "mir/input/device.h"
#include "mir/input/mir_keyboard_config.h"

namespace miroil {
    
InputDeviceObserver::~InputDeviceObserver()
{
}
        
void InputDeviceObserver::apply_keymap(const std::shared_ptr<mir::input::Device> &device, const std::string & layout, const std::string & variant)
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
    

MirInputDeviceId InputDeviceObserver::get_device_id(const std::shared_ptr<mir::input::Device> &device)
{
    return device->id();
}

std::string InputDeviceObserver::get_device_name(const std::shared_ptr<mir::input::Device> &device)
{
    return device->name();
}
    
bool InputDeviceObserver::is_keyboard(const std::shared_ptr<mir::input::Device> &device)
{
    return mir::contains(device->capabilities(), mir::input::DeviceCapability::keyboard);
}
        
bool InputDeviceObserver::is_alpha_numeric(const std::shared_ptr<mir::input::Device> &device)
{
    return mir::contains(device->capabilities(), mir::input::DeviceCapability::alpha_numeric);
}

}
