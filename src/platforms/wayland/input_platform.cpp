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

#include "mir/dispatch/multiplexing_dispatchable.h"
#include "mir/input/input_device_info.h"
#include "mir/input/input_device_registry.h"

namespace md = mir::dispatch;
namespace miw = mir::input::wayland;

miw::InputPlatform::InputPlatform(std::shared_ptr<InputDeviceRegistry> const& input_device_registry) :
    dispatchable_(std::make_shared<mir::dispatch::MultiplexingDispatchable>()),
    registry(input_device_registry),
    keyboard(std::make_shared<InputDevice>(InputDeviceInfo{"keyboard-device", "key-dev-1", DeviceCapability::keyboard})),
    pointer(std::make_shared<InputDevice>(InputDeviceInfo{"mouse-device", "mouse-dev-1", DeviceCapability::pointer})),
    touch(std::make_shared<InputDevice>(InputDeviceInfo{"touch-device", "touch-dev-1", DeviceCapability::touchscreen}))
{
    puts(__PRETTY_FUNCTION__);
}

void miw::InputPlatform::start()
{
    puts(__PRETTY_FUNCTION__);
    registry->add_device(keyboard);
    registry->add_device(pointer);
    registry->add_device(touch);
}

std::shared_ptr<md::Dispatchable> miw::InputPlatform::dispatchable()
{
    puts(__PRETTY_FUNCTION__);
    return dispatchable_;
}

void miw::InputPlatform::stop()
{
    puts(__PRETTY_FUNCTION__);
    registry->remove_device(touch);
    registry->remove_device(pointer);
    registry->remove_device(keyboard);
}

void miw::InputPlatform::pause_for_config()
{
    puts(__PRETTY_FUNCTION__);
}

void miw::InputPlatform::continue_after_config()
{
    puts(__PRETTY_FUNCTION__);
}
