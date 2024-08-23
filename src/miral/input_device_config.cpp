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
#include "input_device_configuration_options.h"

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


template<double lo, double hi>
auto clamp(std::optional<double> opt_val)-> std::optional<double>
{
    return opt_val.transform([](auto val) { return std::clamp(val, lo, hi); });
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
                                    "Set the pointer acceleration speed of mice within a range of [-1.0, 1.0]",
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
                                    "Set the pointer acceleration speed of touchpads within a range of [-1.0, 1.0]",
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
            auto const input_config = InputDeviceConfig::the_input_configuration(server.get_options());
            server.the_input_device_hub()->add_observer(input_config);
        });
}

miral::InputDeviceConfig::InputDeviceConfig(std::shared_ptr<mir::options::Option> const& options) :
    mouse_config{
        to_handedness(get_optional<std::string>(options, mouse_handedness_opt)),
        to_acceleration_profile(get_optional<std::string>(options, mouse_cursor_acceleration_opt)),
        clamp<-1.0, 1.0>(get_optional<double>(options, mouse_cursor_acceleration_bias_opt)),
        get_optional<double>(options, mouse_vscroll_speed_override_opt, mouse_scroll_speed_opt),
        get_optional<double>(options, mouse_hscroll_speed_override_opt, mouse_scroll_speed_opt)
    },
    touchpad_config{
        get_optional<bool>(options, disable_while_typing_opt),
        get_optional<bool>(options, disable_with_external_mouse_opt),
        to_acceleration_profile(get_optional<std::string>(options, touchpad_cursor_acceleration_opt)),
        clamp<-1.0, 1.0>(get_optional<double>(options, touchpad_cursor_acceleration_bias_opt)),
        get_optional<double>(options, touchpad_vscroll_speed_override_opt, touchpad_scroll_speed_opt),
        get_optional<double>(options, touchpad_hscroll_speed_override_opt, touchpad_scroll_speed_opt),
        convert_to_click_mode(get_optional<std::string>(options, touchpad_click_mode_opt)),
        convert_to_scroll_mode(get_optional<std::string>(options, touchpad_scroll_mode_opt)),
        get_optional<bool>(options, touchpad_tap_to_click_opt),
        get_optional<bool>(options, touchpad_middle_mouse_button_emulation_opt)
    }
{
}

auto miral::InputDeviceConfig::the_input_configuration(std::shared_ptr<mir::options::Option> const& options)
-> std::shared_ptr<InputDeviceConfig>
{
    // This should only be called in single-threaded phase of startup, so no need for synchronization
    static std::weak_ptr<InputDeviceConfig> instance;

    if (auto result = instance.lock())
    {
        return result;
    }

    std::shared_ptr<InputDeviceConfig> result{new InputDeviceConfig(options)};
    instance = result;
    return result;
}

void miral::InputDeviceConfig::device_added(std::shared_ptr<mi::Device> const& device)
{
    touchpad_config.apply_to(*device);
    mouse_config.apply_to(*device);
}

auto miral::InputDeviceConfig::mouse() const -> MouseInputConfiguration
{
    return mouse_config;
}

auto miral::InputDeviceConfig::touchpad() const -> TouchpadInputConfiguration
{
    return touchpad_config;
}

void miral::InputDeviceConfig::mouse(MouseInputConfiguration const& val)
{
    mouse_config = val;
}

void miral::InputDeviceConfig::touchpad(TouchpadInputConfiguration const& val)
{
    touchpad_config = val;
}

void miral::TouchpadInputConfiguration::apply_to(mi::Device& device) const
{
    if (contains(device.capabilities(), mi::DeviceCapability::touchpad))
    {
        mir::log_debug("Configuring touchpad: '%s'", device.name().c_str());
        if (auto const optional_pointer_config = device.pointer_configuration(); optional_pointer_config.is_set())
        {
            MirPointerConfig pointer_config( optional_pointer_config.value() );
            if (acceleration) pointer_config.acceleration(*acceleration);
            if (acceleration_bias) pointer_config.cursor_acceleration_bias(*acceleration_bias);
            if (vscroll_speed) pointer_config.vertical_scroll_scale(*vscroll_speed);
            if (hscroll_speed) pointer_config.horizontal_scroll_scale(*hscroll_speed);
            device.apply_pointer_configuration(pointer_config);
        }

        if (auto const optional_touchpad_config = device.touchpad_configuration(); optional_touchpad_config.is_set())
        {
            MirTouchpadConfig touch_config( optional_touchpad_config.value() );
            if (disable_while_typing) touch_config.disable_while_typing(*disable_while_typing);
            if (disable_with_external_mouse) touch_config.disable_while_typing(*disable_with_external_mouse);
            if (click_mode) touch_config.click_mode(*click_mode);
            if (scroll_mode) touch_config.scroll_mode(*scroll_mode);
            if (tap_to_click) touch_config.tap_to_click(*tap_to_click);
            if (middle_button_emulation) touch_config.middle_mouse_button_emulation(*middle_button_emulation);

            device.apply_touchpad_configuration(touch_config);
        }
    }
}

void miral::MouseInputConfiguration::apply_to(mi::Device& device) const
{
    if (contains(device.capabilities(), mi::DeviceCapability::pointer))
    {
        mir::log_debug("Configuring pointer: '%s'", device.name().c_str());
        if (auto optional_pointer_config = device.pointer_configuration(); optional_pointer_config.is_set())
        {
            MirPointerConfig pointer_config( optional_pointer_config.value() );
            if (handedness) pointer_config.handedness(*handedness);
            if (acceleration) pointer_config.acceleration(*acceleration);
            if (acceleration_bias) pointer_config.cursor_acceleration_bias(*acceleration_bias);
            if (vscroll_speed) pointer_config.vertical_scroll_scale(*vscroll_speed);
            if (hscroll_speed) pointer_config.horizontal_scroll_scale(*hscroll_speed);
            device.apply_pointer_configuration(pointer_config);
        }
    }
}

