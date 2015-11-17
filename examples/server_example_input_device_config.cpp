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
 * Authored By: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "server_example_input_device_config.h"

#include "mir/input/device_capability.h"
#include "mir/input/pointer_configuration.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/device.h"
#include "mir/options/option.h"
#include "mir/server.h"

namespace me = mir::examples;
namespace mi = mir::input;

///\example server_example_input_device_config.cpp
/// Demonstrate input device configuration

char const* const me::disable_while_typing_opt = "disable-while-typing";
char const* const me::mouse_cursor_accleration_bias_opt = "mouse-cursor-acceleration-bias";
char const* const me::mouse_scroll_speed_scale_opt = "mouse-scroll-speed-scale";
char const* const me::touchpad_cursor_accleration_bias_opt = "touchpad-cursor-acceleration-bias";
char const* const me::touchpad_scroll_speed_scale_opt = "touchpad-scroll-speed-scale";

void me::add_input_device_configuration_options_to(mir::Server& server)
{
    // Add choice of monitor configuration
    server.add_configuration_option(disable_while_typing_opt,
                                    "Disable touch pad while typing on keyboard configuration [true, false]",
                                    false);
    server.add_configuration_option(mouse_cursor_accleration_bias_opt,
                                    "Bias to the acceleration curve within the range [-1.0, 1.0] for mice",
                                    0.0);
    server.add_configuration_option(mouse_scroll_speed_scale_opt,
                                    "Scales mice scroll events, use negative values for natural scrolling",
                                    1.0);
    server.add_configuration_option(touchpad_cursor_accleration_bias_opt,
                                    "Bias to the acceleration curve within the range [-1.0, 1.0] for touchpads",
                                    0.0);
    server.add_configuration_option(touchpad_scroll_speed_scale_opt,
                                    "Scales touchpad scroll events, use negative values for natural scrolling",
                                    -1.0);

    server.add_init_callback([&server]()
        {
            auto const options = server.get_options();
            auto const input_config = std::make_shared<me::InputDeviceConfig>(
                options->get<bool>(disable_while_typing_opt),
                options->get<double>(mouse_cursor_accleration_bias_opt),
                options->get<double>(mouse_scroll_speed_scale_opt),
                options->get<double>(touchpad_cursor_accleration_bias_opt),
                options->get<double>(touchpad_scroll_speed_scale_opt)
                );
            server.the_input_device_hub()->add_observer(input_config);
        });
}

///\example server_example_input_event_filter.cpp
/// Demonstrate a custom input by making Ctrl+Alt+BkSp stop the server

me::InputDeviceConfig::InputDeviceConfig(bool disable_while_typing,
                                         double mouse_cursor_accleration_bias,
                                         double mouse_scroll_speed_scale,
                                         double touchpad_scroll_speed_scale,
                                         double touchpad_cursor_accleration_bias)
    : disable_while_typing(disable_while_typing), mouse_cursor_accleration_bias(mouse_cursor_accleration_bias),
      mouse_scroll_speed_scale(mouse_scroll_speed_scale),
      touchpad_cursor_accleration_bias(touchpad_cursor_accleration_bias),
      touchpad_scroll_speed_scale(touchpad_scroll_speed_scale)
{
}

void me::InputDeviceConfig::device_added(std::shared_ptr<mi::Device> const& device)
{
    if (contains(device->capabilities(), mi::DeviceCapability::pointer) &&
        contains(device->capabilities(), mi::DeviceCapability::touchpad))
    {
        mi::PointerConfiguration pointer_config( device->pointer_configuration().value() );
        pointer_config.cursor_acceleration_bias = touchpad_cursor_accleration_bias;
        pointer_config.vertical_scroll_scale  = touchpad_scroll_speed_scale;
        pointer_config.horizontal_scroll_scale = touchpad_scroll_speed_scale;
        device->apply_pointer_configuration(pointer_config);

        mi::TouchpadConfiguration touch_config( device->touchpad_configuration().value() );
        touch_config.disable_while_typing = disable_while_typing;
        device->apply_touchpad_configuration(touch_config);
    }
    else if (contains(device->capabilities(), mi::DeviceCapability::pointer))
    {
        mi::PointerConfiguration pointer_config( device->pointer_configuration().value() );
        pointer_config.cursor_acceleration_bias = mouse_cursor_accleration_bias;
        pointer_config.vertical_scroll_scale  = mouse_scroll_speed_scale;
        pointer_config.horizontal_scroll_scale = mouse_scroll_speed_scale;
        device->apply_pointer_configuration(pointer_config);
    }
}
