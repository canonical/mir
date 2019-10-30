/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 */

#include "input_platform.h"
#include "input_device.h"

#include <mir/dispatch/action_queue.h>
#include <mir/input/input_device_info.h>
#include <mir/input/input_device_registry.h>

namespace md = mir::dispatch;
namespace miw = mir::input::wayland;

miw::InputPlatform::InputPlatform(std::shared_ptr<InputDeviceRegistry> const& input_device_registry) :
    action_queue(std::make_shared<md::ActionQueue>()),
    registry(input_device_registry),
    keyboard(std::make_shared<InputDevice>(InputDeviceInfo{"keyboard-device", "key-dev-1", DeviceCapability::keyboard}, action_queue)),
    pointer(std::make_shared<InputDevice>(InputDeviceInfo{"mouse-device", "mouse-dev-1", DeviceCapability::pointer}, action_queue)),
    touch(std::make_shared<InputDevice>(InputDeviceInfo{"touch-device", "touch-dev-1", DeviceCapability::touchscreen}, action_queue))
{
    puts(__PRETTY_FUNCTION__);
    graphics::wayland::the_display->set_keyboard_sink(keyboard);
    graphics::wayland::the_display->set_pointer_sink(pointer);
}

void miw::InputPlatform::start()
{
    registry->add_device(keyboard);
    registry->add_device(pointer);
    registry->add_device(touch);
}

auto miw::InputPlatform::dispatchable() -> std::shared_ptr<md::Dispatchable>
{
    return action_queue;
}

void miw::InputPlatform::stop()
{
    registry->remove_device(touch);
    registry->remove_device(pointer);
    registry->remove_device(keyboard);
}

void miw::InputPlatform::pause_for_config()
{
}

void miw::InputPlatform::continue_after_config()
{
}
