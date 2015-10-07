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
#include "mir/input/touch_pad_configuration.h"
#include "mir/input/pointer_configuration.h"

namespace mi = mir::input;

mi::DefaultDevice::DefaultDevice(MirInputDeviceId id, std::shared_ptr<dispatch::ActionQueue> const& actions,
                                 std::shared_ptr<InputDevice> const& device) :
    device_id{id}, device{device}, info(device->get_device_info()), pointer{device->get_pointer_settings()}, touch_pad{device->get_touch_pad_settings()}, actions{actions}
{
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

    return PointerConfiguration(settings.primary_button, settings.cursor_speed, settings.horizontal_scroll_speed, settings.vertical_scroll_speed);
}

mir::optional_value<mi::TouchPadConfiguration> mi::DefaultDevice::touch_pad_configuration() const
{
    if (!touch_pad.is_set())
        return {};

    auto const& settings = touch_pad.value();

    return TouchPadConfiguration(settings.click_mode, settings.scroll_mode, settings.button_down_scroll_button,
                                 settings.tap_to_click, settings.disable_while_typing, settings.disable_with_mouse,
                                 settings.middle_mouse_button_emulation);
}

void mi::DefaultDevice::apply_configuration(mi::PointerConfiguration const& conf)
{
    if (!pointer.is_set())
        return;

    PointerSettings settings;
    settings.primary_button = conf.primary_button();
    settings.cursor_speed = conf.cursor_speed();
    settings.vertical_scroll_speed = conf.vertical_scroll_speed();
    settings.horizontal_scroll_speed = conf.horizontal_scroll_speed();

    pointer = settings;

    actions->enqueue([settings = std::move(settings), dev=device]
                     {
                         dev->apply_settings(settings);
                     });
}

void mi::DefaultDevice::apply_configuration(mi::TouchPadConfiguration const& conf)
{
    if (!touch_pad.is_set())
        return;

    TouchPadSettings settings;
    settings.click_mode = conf.click_mode();
    settings.scroll_mode = conf.scroll_mode();
    settings.button_down_scroll_button = conf.scroll_button();
    settings.disable_with_mouse = conf.disable_with_mouse();
    settings.disable_while_typing = conf.disable_while_typing();
    settings.tap_to_click = conf.tap_to_click();
    settings.middle_mouse_button_emulation = conf.middle_mouse_button_emulation();

    touch_pad = settings;

    actions->enqueue([settings = std::move(settings), dev=device]
                     {
                         dev->apply_settings(settings);
                     });
}
