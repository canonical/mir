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

#include "input_device_config.h"

#include "mir/abnormal_exit.h"
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

#include <format>
#include <optional>

namespace mi = mir::input;

namespace
{
template<typename Type>
auto get_optional(std::shared_ptr<mir::options::Option> options, char const* name)
{
    return options->is_set(name) ? std::make_optional<Type>(options->get<Type>(name)) : std::nullopt;
};

template<typename Type>
auto get_optional(std::shared_ptr<mir::options::Option> options, char const* name, char const* alt_name)
{
    return options->is_set(name) ?
        std::make_optional<Type>(options->get<Type>(name)) :
        get_optional<Type>(options, alt_name);
};

char const* const disable_while_typing_opt = "touchpad-disable-while-typing";
char const* const disable_with_external_mouse_opt = "touchpad-disable-with-external-mouse";
char const* const touchpad_tap_to_click_opt = "touchpad-tap-to-click";
char const* const mouse_handedness_opt = "mouse-handedness";
char const* const right = "right";
char const* const left = "left";
char const* const mouse_cursor_acceleration_opt = "mouse-cursor-acceleration";
char const* const acceleration_none = "none";
char const* const acceleration_adaptive = "adaptive";
char const* const mouse_cursor_acceleration_bias_opt = "mouse-cursor-acceleration-bias";
char const* const mouse_scroll_speed_opt = "mouse-scroll-speed";
char const* const mouse_hscroll_speed_override_opt = "mouse-horizontal-scroll-speed-override";
char const* const mouse_vscroll_speed_override_opt = "mouse-vertical-scroll-speed-override";
char const* const touchpad_cursor_acceleration_opt = "touchpad-cursor-acceleration";
char const* const touchpad_cursor_acceleration_bias_opt = "touchpad-cursor-acceleration-bias";
char const* const touchpad_scroll_speed_opt = "touchpad-scroll-speed";
char const* const touchpad_hscroll_speed_override_opt = "touchpad-horizontal-scroll-speed-override";
char const* const touchpad_vscroll_speed_override_opt = "touchpad-vertical-scroll-speed-override";
char const* const touchpad_scroll_mode_opt = "touchpad-scroll-mode";

char const* const touchpad_scroll_mode_two_finger = "two-finger";
char const* const touchpad_scroll_mode_button_down_scroll = "button-down";
char const* const touchpad_scroll_mode_edge = "edge";
char const* const touchpad_scroll_mode_none = "none";

char const* const touchpad_click_mode_opt= "touchpad-click-mode";

char const* const touchpad_click_mode_none = "none";
char const* const touchpad_click_mode_area = "area";
char const* const touchpad_click_mode_clickfinger = "clickfinger";
char const* const touchpad_middle_mouse_button_emulation_opt= "touchpad-middle-mouse-button-emulation";

class InputDeviceConfig : public mi::InputDeviceObserver
{
public:
    explicit InputDeviceConfig(std::shared_ptr<mir::options::Option> const& options);
    void device_added(std::shared_ptr<mi::Device> const& device) override;
    void device_changed(std::shared_ptr<mi::Device> const&) override {}
    void device_removed(std::shared_ptr<mi::Device> const&) override {}
    void changes_complete() override {}
private:
    std::optional<bool> const disable_while_typing;
    std::optional<bool> const disable_with_external_mouse;
    std::optional<MirPointerHandedness> const mouse_handedness;
    std::optional<MirPointerAcceleration> const mouse_cursor_acceleration;
    std::optional<double> const mouse_cursor_acceleration_bias;
    std::optional<double> const mouse_vscroll_speed;
    std::optional<double> const mouse_hscroll_speed;
    std::optional<MirPointerAcceleration> const touchpad_cursor_acceleration;
    std::optional<double> const touchpad_cursor_acceleration_bias;
    std::optional<double> const touchpad_vscroll_speed;
    std::optional<double> const touchpad_hscroll_speed;
    std::optional<MirTouchpadClickMode> const click_mode;
    std::optional<MirTouchpadScrollMode> const scroll_mode;
    std::optional<bool> const tap_to_click;
    std::optional<bool> const middle_mouse_button_emulation;
};

auto clamp_to_range(std::optional<double> opt_val)-> std::optional<double>
{
    if (opt_val)
    {
        auto val = *opt_val;
        if (val < -1.0)
            val = -1.0;
        else if (val > 1.0)
            val = 1.0;
        return std::make_optional(val);
    }
    else
    {
        return std::nullopt;
    }
}

auto convert_to_scroll_mode(std::optional<std::string> const& opt_val)
-> std::optional<MirTouchpadScrollMode>
{
    if (opt_val)
    {
        if (*opt_val == touchpad_scroll_mode_none)
        {
            return mir_touchpad_scroll_mode_none;
        }
        else if (*opt_val == touchpad_scroll_mode_edge)
        {
            return mir_touchpad_scroll_mode_edge_scroll;
        }
        else if (*opt_val == touchpad_scroll_mode_two_finger)
        {
            return mir_touchpad_scroll_mode_two_finger_scroll;
        }
        else if (*opt_val == touchpad_scroll_mode_button_down_scroll)
        {
            return mir_touchpad_scroll_mode_button_down_scroll;
        }
        else
        {
            throw mir::AbnormalExit{std::format("Unrecognised option for {}: {}", touchpad_scroll_mode_opt, *opt_val)};
        }
    }
    else
    {
        return std::nullopt;
    }
}

auto to_acceleration_profile(std::optional<std::string> const& opt_val)-> std::optional<MirPointerAcceleration>
{
    if (opt_val)
    {
        if (*opt_val == acceleration_none)
        {
            return mir_pointer_acceleration_none;
        }
        else if (*opt_val == acceleration_adaptive)
        {
            return mir_pointer_acceleration_adaptive;
        }
        else
        {
            throw mir::AbnormalExit{std::format("Unrecognised option for cursor-acceleration: {}", *opt_val)};
        }
    }
    else
    {
        return std::nullopt;
    }
}

auto convert_to_click_mode(std::optional<std::string> const& opt_val)
-> std::optional<MirTouchpadClickMode>
{
    if (opt_val)
    {
        auto val = *opt_val;
        if (val == touchpad_click_mode_none)
        {
            return mir_touchpad_click_mode_none;
        }
        else if (val == touchpad_click_mode_clickfinger)
        {
            return mir_touchpad_click_mode_finger_count;
        }
        else if (val == touchpad_click_mode_area)
        {
            return mir_touchpad_click_mode_area_to_click;
        }
        else
        {
            throw mir::AbnormalExit{std::format("Unrecognised option for {}: {}", touchpad_click_mode_opt, *opt_val)};
        }
    }
    else
    {
        return std::nullopt;
    }
}

auto to_handedness(std::optional<std::string> const& opt_val)-> std::optional<MirPointerHandedness>
{
    if (opt_val)
    {
        if (*opt_val == right)
        {
            return mir_pointer_handedness_right;
        }
        else if (*opt_val == left)
        {
            return mir_pointer_handedness_left;
        }
        else
        {
            throw mir::AbnormalExit{std::format("Unrecognised option for handedness: {}", *opt_val)};
        }
    }
    else
    {
        return std::nullopt;
    }
}
}

void miral::add_input_device_configuration_options_to(mir::Server& server)
{
    server.add_configuration_option(mouse_handedness_opt, std::format("Select mouse laterality: [{}, {}]", right, left),
    mir::OptionType::string);
    server.add_configuration_option(mouse_cursor_acceleration_opt,
                                    "Select acceleration profile for mice and trackballs [none, adaptive]",
                                    mir::OptionType::string);
    server.add_configuration_option(mouse_cursor_acceleration_bias_opt,
                                    "Constant factor (+1) to velocity or bias to the acceleration curve within the range [-1.0, 1.0] for mice",
                                    mir::OptionType::real);
    server.add_configuration_option(mouse_scroll_speed_opt,
                                    "Scales mouse scroll. Use negative values for natural scrolling",
                                    mir::OptionType::real);
    server.add_configuration_option(mouse_hscroll_speed_override_opt,
                                    "Scales mouse horizontal scroll. Use negative values for natural scrolling",
                                    mir::OptionType::real);
    server.add_configuration_option(mouse_vscroll_speed_override_opt,
                                    "Scales mouse vertical scroll. Use negative values for natural scrolling",
                                    mir::OptionType::real);
    server.add_configuration_option(disable_while_typing_opt,
                                    "Disable touchpad while typing on keyboard configuration [true, false]",
                                    mir::OptionType::boolean);
    server.add_configuration_option(disable_with_external_mouse_opt,
                                    "Disable touchpad if an external pointer device is plugged in [true, false]",
                                    mir::OptionType::boolean);
    server.add_configuration_option(touchpad_tap_to_click_opt,
                                    "Enable or disable tap-to-click on this device, with"
                                    " 1, 2, 3 finger tap mapping to left, right, middle click, respectively [true, false]",
                                    mir::OptionType::boolean);
    server.add_configuration_option(touchpad_cursor_acceleration_opt,
                                    "Select acceleration profile for touchpads [none, adaptive]",
                                    mir::OptionType::string);
    server.add_configuration_option(touchpad_cursor_acceleration_bias_opt,
                                    "Constant factor (+1) to velocity or bias to the acceleration curve within the range [-1.0, 1.0] for touchpads",
                                    mir::OptionType::real);
    server.add_configuration_option(touchpad_scroll_speed_opt,
                                    "Scales touchpad scroll. Use negative values for natural scrolling",
                                    mir::OptionType::real);
    server.add_configuration_option(touchpad_hscroll_speed_override_opt,
                                    "Scales touchpad horizontal scroll. Use negative values for natural scrolling",
                                    mir::OptionType::real);
    server.add_configuration_option(touchpad_vscroll_speed_override_opt,
                                    "Scales touchpad vertical scroll. Use negative values for natural scrolling",
                                    mir::OptionType::real);

    server.add_configuration_option(touchpad_scroll_mode_opt,
                                    std::format("Select scroll mode for touchpads: [{}, {}, {}]",
                                                touchpad_scroll_mode_edge,
                                                touchpad_scroll_mode_two_finger,
                                                touchpad_scroll_mode_button_down_scroll),
                                    mir::OptionType::string);

    server.add_configuration_option(touchpad_click_mode_opt,
                                    std::format("Select click mode for touchpads: [{}, {}, {}]",
                                        touchpad_click_mode_none,
                                        touchpad_click_mode_area,
                                        touchpad_click_mode_clickfinger),
                                    mir::OptionType::string);

    server.add_configuration_option(touchpad_middle_mouse_button_emulation_opt,
                                    "Converts a simultaneous left and right button click into a middle button",
                                    mir::OptionType::boolean);

    server.add_init_callback([&]()
        {
            auto const input_config = std::make_shared<InputDeviceConfig>(server.get_options());
            server.the_input_device_hub()->add_observer(input_config);
        });
}

InputDeviceConfig::InputDeviceConfig(std::shared_ptr<mir::options::Option> const& options) :
    disable_while_typing{get_optional<bool>(options, disable_while_typing_opt)},
    disable_with_external_mouse{get_optional<bool>(options, disable_with_external_mouse_opt)},
    mouse_handedness{to_handedness(get_optional<std::string>(options, mouse_handedness_opt))},
    mouse_cursor_acceleration{to_acceleration_profile(get_optional<std::string>(options, mouse_cursor_acceleration_opt))},
    mouse_cursor_acceleration_bias{clamp_to_range(get_optional<double>(options, mouse_cursor_acceleration_bias_opt))},
    mouse_vscroll_speed{get_optional<double>(options, mouse_vscroll_speed_override_opt, mouse_scroll_speed_opt)},
    mouse_hscroll_speed{get_optional<double>(options, mouse_hscroll_speed_override_opt, mouse_scroll_speed_opt)},
    touchpad_cursor_acceleration{to_acceleration_profile(get_optional<std::string>(options, touchpad_cursor_acceleration_opt))},
    touchpad_cursor_acceleration_bias{clamp_to_range(get_optional<double>(options, touchpad_cursor_acceleration_bias_opt))},
    touchpad_vscroll_speed{get_optional<double>(options, touchpad_vscroll_speed_override_opt, touchpad_scroll_speed_opt)},
    touchpad_hscroll_speed{get_optional<double>(options, touchpad_hscroll_speed_override_opt, touchpad_scroll_speed_opt)},
    click_mode{convert_to_click_mode(get_optional<std::string>(options, touchpad_click_mode_opt))},
    scroll_mode{convert_to_scroll_mode(get_optional<std::string>(options, touchpad_scroll_mode_opt))},
    tap_to_click{get_optional<bool>(options, touchpad_tap_to_click_opt)},
    middle_mouse_button_emulation{get_optional<bool>(options, touchpad_middle_mouse_button_emulation_opt)}
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
            if (touchpad_cursor_acceleration) pointer_config.acceleration(*touchpad_cursor_acceleration);
            if (touchpad_cursor_acceleration_bias) pointer_config.cursor_acceleration_bias(*touchpad_cursor_acceleration_bias);
            if (touchpad_vscroll_speed) pointer_config.vertical_scroll_scale(*touchpad_vscroll_speed);
            if (touchpad_hscroll_speed) pointer_config.horizontal_scroll_scale(*touchpad_hscroll_speed);
            device->apply_pointer_configuration(pointer_config);
        }

	    if (auto const optional_touchpad_config = device->touchpad_configuration(); optional_touchpad_config.is_set())
        {
            MirTouchpadConfig touch_config( optional_touchpad_config.value() );
            if (disable_while_typing) touch_config.disable_while_typing(*disable_while_typing);
            if (disable_with_external_mouse) touch_config.disable_while_typing(*disable_with_external_mouse);
	        if (click_mode) touch_config.click_mode(*click_mode);
            if (scroll_mode) touch_config.scroll_mode(*scroll_mode);
            if (tap_to_click) touch_config.tap_to_click(*tap_to_click);
            if (middle_mouse_button_emulation) touch_config.middle_mouse_button_emulation(*middle_mouse_button_emulation);

            device->apply_touchpad_configuration(touch_config);
        }
    }
    else if (contains(device->capabilities(), mi::DeviceCapability::pointer))
    {
        mir::log_debug("Configuring pointer: '%s'", device->name().c_str());
        if (auto optional_pointer_config = device->pointer_configuration(); optional_pointer_config.is_set())
        {
            MirPointerConfig pointer_config( optional_pointer_config.value() );
            if (mouse_handedness) pointer_config.handedness(*mouse_handedness);
            if (mouse_cursor_acceleration) pointer_config.acceleration(*mouse_cursor_acceleration);
            if (mouse_cursor_acceleration_bias) pointer_config.cursor_acceleration_bias(*mouse_cursor_acceleration_bias);
            if (mouse_vscroll_speed) pointer_config.vertical_scroll_scale(*mouse_vscroll_speed);
            if (mouse_hscroll_speed) pointer_config.horizontal_scroll_scale(*mouse_hscroll_speed);
            device->apply_pointer_configuration(pointer_config);
        }
    }
}
