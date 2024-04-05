/*
 * Copyright © Canonical Ltd.
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
 */

#include "miral/input_device_config.h"

#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/input_device_observer.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/options/option.h"
#include "mir/server.h"
#include "mir_toolkit/mir_input_device_types.h"
#include "mir/log.h"

namespace mi = mir::input;

namespace
{
class InputDeviceConfig : public mir::input::InputDeviceObserver
{
public:
    InputDeviceConfig(bool disable_while_typing,
                      bool tap_to_click,
                      MirPointerAcceleration mouse_profile,
                      double mouse_cursor_acceleration_bias,
                      double mouse_scroll_speed_scale,
                      double touchpad_cursor_acceleration_bias,
                      double touchpad_scroll_speed_scale,
                      MirTouchpadClickModes click_mode,
                      MirTouchpadScrollModes scroll_mode);
    void device_added(std::shared_ptr<mi::Device> const& device) override;
    void device_changed(std::shared_ptr<mi::Device> const&) override {}
    void device_removed(std::shared_ptr<mi::Device> const&) override {}
    void changes_complete() override {}
private:
    bool const disable_while_typing;
    MirPointerAcceleration const mouse_profile;
    double const mouse_cursor_acceleration_bias;
    double const mouse_scroll_speed_scale;
    double const touchpad_cursor_acceleration_bias;
    double const touchpad_scroll_speed_scale;
    MirTouchpadClickModes const click_mode;
    MirTouchpadScrollModes const scroll_mode;
    bool const tap_to_click;
};

char const* const disable_while_typing_opt = "touchpad-disable-while-typing";
char const* const tap_to_click_opt = "touchpad-tap-to-click";
char const* const mouse_acceleration_opt = "mouse-acceleration";
char const* const acceleration_none = "none";
char const* const acceleration_adaptive = "adaptive";
char const* const mouse_cursor_acceleration_bias_opt = "mouse-cursor-acceleration-bias";
char const* const mouse_scroll_speed_scale_opt = "mouse-scroll-speed-scale";
char const* const touchpad_cursor_acceleration_bias_opt = "touchpad-cursor-acceleration-bias";
char const* const touchpad_scroll_speed_scale_opt = "touchpad-scroll-speed-scale";
char const* const touchpad_scroll_mode_opt = "touchpad-scroll-mode";

char const* const touchpad_scroll_mode_two_finger = "two-finger";
char const* const touchpad_scroll_mode_edge = "edge";

char const* const touchpad_click_mode_opt= "touchpad-click-mode";

char const* const touchpad_click_mode_area = "area";
char const* const touchpad_click_mode_finger_count = "finger-count";
}

void miral::add_input_device_configuration(::mir::Server& server)
{
    // Add choice of monitor configuration
    server.add_configuration_option(disable_while_typing_opt,
                                    "Disable touchpad while typing on keyboard configuration [true, false]",
                                    false);
    server.add_configuration_option(tap_to_click_opt,
                                    "Enable or disable tap-to-click on this device, with a default mapping of"
                                    " 1, 2, 3 finger tap mapping to left, right, middle click, respectively [true, false]",
                                    false);
    server.add_configuration_option(mouse_acceleration_opt,
                                    "Select acceleration profile for mice and trackballs [none, adaptive]",
                                    acceleration_adaptive);
    server.add_configuration_option(mouse_cursor_acceleration_bias_opt,
                                    "Constant factor (+1) to velocity or bias to the acceleration curve within the range [-1.0, 1.0] for mice",
                                    0.0);
    server.add_configuration_option(mouse_scroll_speed_scale_opt,
                                    "Scales mice scroll events, use negative values for natural scrolling",
                                    1.0);
    server.add_configuration_option(touchpad_cursor_acceleration_bias_opt,
                                    "Constant factor (+1) to velocity or bias to the acceleration curve within the range [-1.0, 1.0] for touchpads",
                                    0.0);
    server.add_configuration_option(touchpad_scroll_speed_scale_opt,
                                    "Scales touchpad scroll events, use negative values for natural scrolling",
                                    -1.0);

    server.add_configuration_option(touchpad_scroll_mode_opt,
                                    "Select scroll mode for touchpads: [{two-finger, edge}]",
                                    touchpad_scroll_mode_two_finger);

    server.add_configuration_option(touchpad_click_mode_opt,
                                    "Select click mode for touchpads: [{area, finger-count}]",
                                    touchpad_click_mode_finger_count);

    auto clamp_to_range = [](double val)
    {
        if (val < -1.0)
            val = -1.0;
        else if (val > 1.0)
            val = 1.0;
        return val;
    };

    // TODO the configuration api allows a combination of values. We might want to expose that in the command line api too.
    auto convert_to_scroll_mode = [](std::string const& val)
    {
        if (val == touchpad_scroll_mode_edge)
            return mir_touchpad_scroll_mode_edge_scroll;
        if (val == touchpad_scroll_mode_two_finger)
            return mir_touchpad_scroll_mode_two_finger_scroll;
        return mir_touchpad_scroll_mode_none;
    };

    auto to_profile = [](std::string const& val)
    {
        if (val == acceleration_none)
            return mir_pointer_acceleration_none;
        return mir_pointer_acceleration_adaptive;
    };

    auto convert_to_click_mode = [](std::string const& val)
    {
        if (val == touchpad_click_mode_finger_count)
            return mir_touchpad_click_mode_finger_count;
        if (val == touchpad_click_mode_area)
            return mir_touchpad_click_mode_area_to_click;
        return mir_touchpad_click_mode_none;
    };

    server.add_init_callback([&]()
        {
            auto const options = server.get_options();
            auto const input_config = std::make_shared<InputDeviceConfig>(
                options->get<bool>(disable_while_typing_opt),
                options->get<bool>(tap_to_click_opt),
                to_profile(options->get<std::string>(mouse_acceleration_opt)),
                clamp_to_range(options->get<double>(mouse_cursor_acceleration_bias_opt)),
                options->get<double>(mouse_scroll_speed_scale_opt),
                clamp_to_range(options->get<double>(touchpad_cursor_acceleration_bias_opt)),
                options->get<double>(touchpad_scroll_speed_scale_opt),
                convert_to_click_mode(options->get<std::string>(touchpad_click_mode_opt)),
                convert_to_scroll_mode(options->get<std::string>(touchpad_scroll_mode_opt))
                );
            server.the_input_device_hub()->add_observer(input_config);
        });
}

InputDeviceConfig::InputDeviceConfig(bool disable_while_typing,
                                         bool tap_to_click,
                                         MirPointerAcceleration mouse_profile,
                                         double mouse_cursor_acceleration_bias,
                                         double mouse_scroll_speed_scale,
                                         double touchpad_cursor_acceleration_bias,
                                         double touchpad_scroll_speed_scale,
                                         MirTouchpadClickModes click_mode,
                                         MirTouchpadClickModes scroll_mode)
    : disable_while_typing{disable_while_typing}, mouse_profile{mouse_profile},
      mouse_cursor_acceleration_bias{mouse_cursor_acceleration_bias},
      mouse_scroll_speed_scale{mouse_scroll_speed_scale},
      touchpad_cursor_acceleration_bias{touchpad_cursor_acceleration_bias},
      touchpad_scroll_speed_scale{touchpad_scroll_speed_scale}, click_mode{click_mode}, scroll_mode{scroll_mode},
      tap_to_click{tap_to_click}
{
}

void InputDeviceConfig::device_added(std::shared_ptr<mi::Device> const& device)
{
    if (contains(device->capabilities(), mi::DeviceCapability::touchpad))
    {
        mir::log_debug("Configuring touchpad: '%s'", device->name().c_str());
	    if (auto const optional_pointer_config = device->pointer_configuration(); optional_pointer_config.is_set())
        {
            MirPointerConfig pointer_config( optional_pointer_config.value() );
            pointer_config.cursor_acceleration_bias(touchpad_cursor_acceleration_bias);
            pointer_config.vertical_scroll_scale(touchpad_scroll_speed_scale);
            pointer_config.horizontal_scroll_scale(touchpad_scroll_speed_scale);
            device->apply_pointer_configuration(pointer_config);
        }

	    if (auto const optional_touchpad_config = device->touchpad_configuration(); optional_touchpad_config.is_set())
        {
            MirTouchpadConfig touch_config( optional_touchpad_config.value() );
            touch_config.disable_while_typing(disable_while_typing);
            touch_config.click_mode(click_mode);
            touch_config.scroll_mode(scroll_mode);
            touch_config.tap_to_click(tap_to_click);
            device->apply_touchpad_configuration(touch_config);
        }
    }
    else if (contains(device->capabilities(), mi::DeviceCapability::pointer))
    {
        mir::log_debug("Configuring pointer: '%s'", device->name().c_str());
        if (auto optional_pointer_config = device->pointer_configuration(); optional_pointer_config.is_set())
        {
            MirPointerConfig pointer_config( optional_pointer_config.value() );
            pointer_config.acceleration(mouse_profile);
            pointer_config.cursor_acceleration_bias(mouse_cursor_acceleration_bias);
            pointer_config.vertical_scroll_scale(mouse_scroll_speed_scale);
            pointer_config.horizontal_scroll_scale(mouse_scroll_speed_scale);
            device->apply_pointer_configuration(pointer_config);
        }
    }
}
