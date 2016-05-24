/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "default_device.h"
#include "mir/dispatch/action_queue.h"
#include "mir/input/device_capability.h"
#include "mir/input/input_device.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/input/pointer_configuration.h"
#include "mir/input/key_mapper.h"

#include <boost/throw_exception.hpp>
#include <stdexcept>

namespace mi = mir::input;

mi::DefaultDevice::DefaultDevice(MirInputDeviceId id, std::shared_ptr<dispatch::ActionQueue> const& actions,
                                 InputDevice& device, std::shared_ptr<KeyMapper> const& key_mapper)
    : device_id{id}, device{device}, info(device.get_device_info()), pointer{device.get_pointer_settings()},
      touchpad{device.get_touchpad_settings()}, actions{actions}, key_mapper{key_mapper}
{
    if (contains(info.capabilities, mi::DeviceCapability::keyboard) &&
        contains(info.capabilities, mi::DeviceCapability::alpha_numeric))
    {
        keyboard = mi::KeyboardConfiguration{};
        key_mapper->set_keymap(device_id, keyboard.value().device_keymap);
    }
}

mi::DeviceCapabilities mi::DefaultDevice::capabilities() const
{
    return info.capabilities;
}

std::string mi::DefaultDevice::name() const
{
    return info.name;
}

std::string mi::DefaultDevice::unique_id() const
{
    return info.unique_id;
}

MirInputDeviceId mi::DefaultDevice::id() const
{
    return device_id;
}

mir::optional_value<mi::PointerConfiguration> mi::DefaultDevice::pointer_configuration() const
{
    if (!pointer.is_set())
        return {};

    auto const& settings = pointer.value();

    return PointerConfiguration(settings.handedness, settings.acceleration,
                                settings.cursor_acceleration_bias, settings.horizontal_scroll_scale,
                                settings.vertical_scroll_scale);
}

mir::optional_value<mi::TouchpadConfiguration> mi::DefaultDevice::touchpad_configuration() const
{
    if (!touchpad.is_set())
        return {};

    auto const& settings = touchpad.value();

    return TouchpadConfiguration(settings.click_mode, settings.scroll_mode, settings.button_down_scroll_button,
                                 settings.tap_to_click, settings.disable_while_typing, settings.disable_with_mouse,
                                 settings.middle_mouse_button_emulation);
}

void mi::DefaultDevice::apply_pointer_configuration(mi::PointerConfiguration const& conf)
{
    if (!pointer.is_set())
        BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot apply a pointer configuration"));

    if (conf.cursor_acceleration_bias < -1.0 || conf.cursor_acceleration_bias > 1.0)
        BOOST_THROW_EXCEPTION(std::invalid_argument("Cursor acceleration bias out of range"));

    PointerSettings settings;
    settings.handedness = conf.handedness;
    settings.acceleration = conf.acceleration;
    settings.cursor_acceleration_bias = conf.cursor_acceleration_bias;
    settings.vertical_scroll_scale = conf.vertical_scroll_scale;
    settings.horizontal_scroll_scale = conf.horizontal_scroll_scale;

    pointer = settings;

    actions->enqueue([settings = std::move(settings), dev=&device]
                     {
                         dev->apply_settings(settings);
                     });
}

void mi::DefaultDevice::apply_touchpad_configuration(mi::TouchpadConfiguration const& conf)
{
    if (!touchpad.is_set())
        BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot apply a touchpad configuration"));

    if (conf.scroll_mode & mir_touchpad_scroll_mode_button_down_scroll && conf.button_down_scroll_button == mi::no_scroll_button)
        BOOST_THROW_EXCEPTION(std::invalid_argument("No scroll button configured"));

    TouchpadSettings settings;
    settings.click_mode = conf.click_mode;
    settings.scroll_mode = conf.scroll_mode;
    settings.button_down_scroll_button = conf.button_down_scroll_button;
    settings.disable_with_mouse = conf.disable_with_mouse;
    settings.disable_while_typing = conf.disable_while_typing;
    settings.tap_to_click = conf.tap_to_click;
    settings.middle_mouse_button_emulation = conf.middle_mouse_button_emulation;

    touchpad = settings;

    actions->enqueue([settings = std::move(settings), dev=&device]
                     {
                         dev->apply_settings(settings);
                     });
}

mir::optional_value<mi::KeyboardConfiguration> mi::DefaultDevice::keyboard_configuration() const
{
    return keyboard;
}

void mi::DefaultDevice::apply_keyboard_configuration(mi::KeyboardConfiguration const& conf)
{
    if (!contains(info.capabilities, mi::DeviceCapability::keyboard))
        BOOST_THROW_EXCEPTION(std::invalid_argument("Cannot apply a keyboard configuration"));

    if (keyboard.value().device_keymap != conf.device_keymap)
    {
        keyboard = conf;
        key_mapper->set_keymap(device_id, conf.device_keymap);
    }
}
