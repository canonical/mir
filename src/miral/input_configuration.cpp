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

#include "miral/input_configuration.h"
#include "miral/live_config.h"

#include "input_device_config.h"

#include <mir/input/input_device_hub.h>
#include <mir/log.h>
#include <mir/server.h>
#include "mir/shell/accessibility_manager.h"

#include <algorithm>
#include <format>
#include <memory>

class miral::InputConfiguration::Mouse::Self : public MouseInputConfiguration
{
    friend class miral::InputConfiguration::Self;
    using MouseInputConfiguration::operator=;
};

class miral::InputConfiguration::Touchpad::Self : public TouchpadInputConfiguration
{
    friend class miral::InputConfiguration::Self;
    using TouchpadInputConfiguration::operator=;
};

class miral::InputConfiguration::Keyboard::Self : public KeyboardInputConfiguration
{
    friend class miral::InputConfiguration::Self;
    using KeyboardInputConfiguration::operator=;
};

class miral::InputConfiguration::Self
{
public:
    std::weak_ptr<mir::input::InputDeviceHub> input_device_hub{};
    std::weak_ptr<InputDeviceConfig> input_device_config;
    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;

    auto mouse() -> Mouse;
    void mouse(Mouse const& val);
    auto touchpad() -> Touchpad;
    void touchpad(Touchpad const& val);
    auto keyboard() -> Keyboard;
    void keyboard(Keyboard const& val);

    void apply(Mouse const& m)
    {
        if (auto const idh = input_device_hub.lock())
        {
            idh->for_each_mutable_input_device([&m](auto& input_device)
            {
                m.self->apply_to(input_device);
            });
        }

        mouse(m);
    }

    void apply(Touchpad const& t)
    {
        if (auto const idh = input_device_hub.lock())
        {
            idh->for_each_mutable_input_device([&t](auto& input_device)
            {
                t.self->apply_to(input_device);
            });
        }

        touchpad(t);
    }

    void apply(Keyboard const& k)
    {
        if(auto const& am = accessibility_manager.lock())
        {
            am->repeat_rate_and_delay(k.self->repeat_rate, k.self->repeat_delay);
        }
        keyboard(k);
    }

    struct Config
    {
        Config(Mouse mouse, Touchpad touchpad, Keyboard keyboard) :
            mouse{mouse},
            touchpad{touchpad},
            keyboard{keyboard}
        {}

        std::mutex config_mutex;
        Mouse mouse;
        Touchpad touchpad;
        Keyboard keyboard;
    } config{mouse(), touchpad(), keyboard()};

    void start_callback()
    {
        // Merge the any input options collected from the command line,
        // environment, and default `.config` file with the options we
        // read from the live configuration file.
        std::lock_guard lock{config.config_mutex};
        config.mouse.merge(mouse());
        config.keyboard.merge(keyboard());
        config.touchpad.merge(touchpad());
    }
};

auto miral::InputConfiguration::Self::mouse() -> Mouse
{
    Mouse result;
    if (auto const& idc = input_device_config.lock())
    {
        *result.self = idc->mouse();
    }
    return result;
}

void miral::InputConfiguration::Self::mouse(Mouse const& val)
{
    if (auto const& idc = input_device_config.lock())
    {
        idc->mouse(*val.self);
    }
}

auto miral::InputConfiguration::Self::touchpad() -> Touchpad
{
    Touchpad result;
    if (auto const& idc = input_device_config.lock())
    {
        *result.self = idc->touchpad();
    }
    return result;
}

void miral::InputConfiguration::Self::touchpad(Touchpad const& val)
{
    if (auto const& idc = input_device_config.lock())
    {
        idc->touchpad(*val.self);
    }
}

auto miral::InputConfiguration::Self::keyboard() -> Keyboard {
    Keyboard result;
    if (auto const& idc = input_device_config.lock())
    {
        *result.self = idc->keyboard();
    }
    return result;
}

void miral::InputConfiguration::Self::keyboard(Keyboard const& val)
{
    if (auto const& idc = input_device_config.lock())
    {
        idc->keyboard(*val.self);
    }
}

miral::InputConfiguration::InputConfiguration() :
    self{std::make_shared<Self>()}
{
}

miral::InputConfiguration::InputConfiguration(live_config::Store& config_store) : InputConfiguration{}
{
        config_store.add_string_attribute(
            {"pointer", "handedness"},
            "Pointer handedness [{right, left}]",
                [this](live_config::Key const& key, std::optional<std::string_view> val)
                {
                    if (val)
                    {
                        std::lock_guard lock{self->config.config_mutex};
                        auto const& value = *val;
                        if (value == "right")
                            self->config.mouse.handedness(mir_pointer_handedness_right);
                        else if (value == "left")
                            self->config.mouse.handedness(mir_pointer_handedness_left);
                        else
                            mir::log_warning(
                                "Config key '%s' has invalid integer value: %s",
                                key.to_string().c_str(),
                                std::format("{}",*val).c_str());
                    }
                });

        config_store.add_string_attribute(
            {"touchpad", "scroll_mode"},
            "Touchpad scroll mode [{none, two_finger_scroll, edge_scroll, button_down_scroll}]",
            [this](live_config::Key const& key, std::optional<std::string_view> val)
            {
                if (val)
                {
                    std::lock_guard lock{self->config.config_mutex};
                    auto const& value = *val;
                    if (value == "none")
                        self->config.touchpad.scroll_mode(mir_touchpad_scroll_mode_none);
                    else if (value == "two_finger_scroll")
                        self->config.touchpad.scroll_mode(mir_touchpad_scroll_mode_two_finger_scroll);
                    else if (value == "edge_scroll")
                        self->config.touchpad.scroll_mode(mir_touchpad_scroll_mode_edge_scroll);
                    else if (value == "button_down_scroll")
                        self->config.touchpad.scroll_mode(mir_touchpad_scroll_mode_button_down_scroll);
                    else
                        mir::log_warning(
                            "Config key '%s' has invalid integer value: %s",
                            key.to_string().c_str(),
                            std::format("{}",*val).c_str());
                }
            });

        config_store.add_int_attribute(
            {"keyboard", "repeat_rate"},
            "Keyboard repeat rate",
            [this](live_config::Key const& key, std::optional<int> val)
            {
                if (val)
                {
                    if (val >= 0)
                    {
                        std::lock_guard lock{self->config.config_mutex};
                        self->config.keyboard.set_repeat_rate(*val);
                    }
                    else
                    {
                        mir::log_warning(
                            "Config value %s does not support negative values. Ignoring the supplied value (%d)...",
                            key.to_string().c_str(), *val);
                    }
                }
            });

        config_store.add_int_attribute(
            {"keyboard", "repeat_delay"},
            "Keyboard repeat delay",
            [this](live_config::Key const& key, std::optional<int> val)
            {
                if (val)
                {
                    if (val >= 0)
                    {
                        std::lock_guard lock{self->config.config_mutex};
                        self->config.keyboard.set_repeat_delay(*val);
                    }
                    else
                    {
                        mir::log_warning(
                            "Config value %s does not support negative values. Ignoring the supplied value (%d)...",
                            key.to_string().c_str(), *val);
                    }
                }
            });

        config_store.on_done([this]
            {
                std::lock_guard lock{self->config.config_mutex};
                mouse(self->config.mouse);
                touchpad(self->config.touchpad);
                keyboard(self->config.keyboard);
            });
}

void miral::InputConfiguration::mouse(Mouse const& m)
{
    self->apply(m);
}

miral::InputConfiguration::~InputConfiguration() = default;

void miral::InputConfiguration::operator()(mir::Server& server)
{
    server.add_init_callback([self=self, &server]
    {
        self->input_device_hub = server.the_input_device_hub();
        self->input_device_config = InputDeviceConfig::the_input_configuration(server.get_options());
        self->accessibility_manager = server.the_accessibility_manager();
        self->start_callback();
    });
}

auto miral::InputConfiguration::mouse() -> Mouse
{
    return self->mouse();
}

auto miral::InputConfiguration::touchpad() -> Touchpad
{
    return self->touchpad();
}

void miral::InputConfiguration::touchpad(Touchpad const& t)
{
    self->apply(t);
}

auto miral::InputConfiguration::keyboard() -> Keyboard
{
    return self->keyboard();
}

void miral::InputConfiguration::keyboard(Keyboard const& k)
{
    self->apply(k);
}

miral::InputConfiguration::Mouse::Mouse() :
    self{std::make_unique<Self>()}
{
}

miral::InputConfiguration::Mouse::Mouse(Mouse const& that) :
    self{std::make_unique<Self>(*that.self)}
{
}

auto miral::InputConfiguration::Mouse::operator=(Mouse that) -> Mouse&
{
    std::swap(self, that.self);
    return *this;
}

miral::InputConfiguration::Mouse::~Mouse() = default;

auto miral::InputConfiguration::Mouse::handedness() const -> std::optional<MirPointerHandedness>
{
    return self->handedness;
}

auto miral::InputConfiguration::Mouse::acceleration() const -> std::optional<MirPointerAcceleration>
{
    return self->acceleration;
}

auto miral::InputConfiguration::Mouse::acceleration_bias() const -> std::optional<double>
{
    return self->acceleration_bias;
}

auto miral::InputConfiguration::Mouse::vscroll_speed() const -> std::optional<double>
{
    return self->vscroll_speed;
}

auto miral::InputConfiguration::Mouse::hscroll_speed() const -> std::optional<double>
{
    return self->hscroll_speed;
}

void miral::InputConfiguration::Mouse::handedness(std::optional<MirPointerHandedness> const& val)
{
    self->handedness = val;
}

void miral::InputConfiguration::Mouse::acceleration(std::optional<MirPointerAcceleration> const& val)
{
    self->acceleration = val;
}

void miral::InputConfiguration::Mouse::acceleration_bias(std::optional<double> const& val)
{
    self->acceleration_bias = val.transform([](auto v) { return std::clamp(v, -1.0, +1.0); });
}

void miral::InputConfiguration::Mouse::vscroll_speed(std::optional<double> const& val)
{
    self->vscroll_speed = val;
}

void miral::InputConfiguration::Mouse::hscroll_speed(std::optional<double> const& val)
{
    self->hscroll_speed = val;
}

miral::InputConfiguration::Touchpad::Touchpad() :
    self{std::make_unique<Self>()}
{
}

miral::InputConfiguration::Touchpad::~Touchpad() = default;

miral::InputConfiguration::Touchpad::Touchpad(Touchpad const& that) :
    self{std::make_unique<Self>(*that.self)}
{
}

auto miral::InputConfiguration::Touchpad::operator=(Touchpad that) -> Touchpad&
{
    std::swap(self, that.self);
    return *this;
}

auto miral::InputConfiguration::Touchpad::disable_while_typing() const -> std::optional<bool>
{
    return self->disable_while_typing;
}

auto miral::InputConfiguration::Touchpad::disable_with_external_mouse() const -> std::optional<bool>
{
    return self->disable_with_external_mouse;
}

auto miral::InputConfiguration::Touchpad::acceleration() const -> std::optional<MirPointerAcceleration>
{
    return self->acceleration;
}

auto miral::InputConfiguration::Touchpad::acceleration_bias() const -> std::optional<double>
{
    return self->acceleration_bias;
}

auto miral::InputConfiguration::Touchpad::vscroll_speed() const -> std::optional<double>
{
    return self->vscroll_speed;
}

auto miral::InputConfiguration::Touchpad::hscroll_speed() const -> std::optional<double>
{
    return self->hscroll_speed;
}

auto miral::InputConfiguration::Touchpad::click_mode() const -> std::optional<MirTouchpadClickMode>
{
    return self->click_mode;
}

auto miral::InputConfiguration::Touchpad::scroll_mode() const -> std::optional<MirTouchpadScrollMode>
{
    return self->scroll_mode;
}

auto miral::InputConfiguration::Touchpad::tap_to_click() const -> std::optional<bool>
{
    return self->tap_to_click;
}

auto miral::InputConfiguration::Touchpad::middle_mouse_button_emulation() const -> std::optional<bool>
{
    return self->middle_button_emulation;
}

void miral::InputConfiguration::Touchpad::disable_while_typing(std::optional<bool> const& val)
{
    self->disable_while_typing = val;
}

void miral::InputConfiguration::Touchpad::disable_with_external_mouse(std::optional<bool> const& val)
{
    self->disable_with_external_mouse = val;
}

void miral::InputConfiguration::Touchpad::acceleration(std::optional<MirPointerAcceleration> const& val)
{
    self->acceleration = val;
}

void miral::InputConfiguration::Touchpad::acceleration_bias(std::optional<double> const& val)
{
    self->acceleration_bias = val.transform([](auto v) { return std::clamp(v, -1.0, +1.0); });
}

void miral::InputConfiguration::Touchpad::vscroll_speed(std::optional<double> const& val)
{
    self->vscroll_speed = val;
}

void miral::InputConfiguration::Touchpad::hscroll_speed(std::optional<double> const& val)
{
    self->hscroll_speed = val;
}


void miral::InputConfiguration::Touchpad::click_mode(std::optional<MirTouchpadClickMode> const& val)
{
    self->click_mode = val;
}

void miral::InputConfiguration::Touchpad::scroll_mode(std::optional<MirTouchpadScrollMode> const& val)
{
    self->scroll_mode = val;
}

void miral::InputConfiguration::Touchpad::tap_to_click(std::optional<bool> const& val)
{
    self->tap_to_click = val;
}

void miral::InputConfiguration::Touchpad::middle_mouse_button_emulation(std::optional<bool> const& val)
{
    self->middle_button_emulation = val;
}

miral::InputConfiguration::Keyboard::Keyboard() :
    self{std::make_unique<Self>()}
{
}

miral::InputConfiguration::Keyboard::~Keyboard() = default;

miral::InputConfiguration::Keyboard::Keyboard(Keyboard const& that) :
    self{std::make_unique<Self>(*that.self)}
{
}

auto miral::InputConfiguration::Keyboard::operator=(Keyboard that) -> Keyboard&
{
    std::swap(self, that.self);
    return *this;
}

void miral::InputConfiguration::Keyboard::set_repeat_rate(int new_rate) {
    self->repeat_rate = new_rate;
}

void miral::InputConfiguration::Keyboard::set_repeat_delay(int new_delay) {
    self->repeat_delay = new_delay;
}

auto miral::InputConfiguration::Keyboard::repeat_rate() const -> std::optional<int>
{
    return self->repeat_rate;
}

auto miral::InputConfiguration::Keyboard::repeat_delay() const -> std::optional<int>
{
    return self->repeat_delay;
}

void miral::InputConfiguration::Mouse::merge(InputConfiguration::Mouse const& other)
{
    self->merge(*other.self);
}

void miral::InputConfiguration::Touchpad::merge(InputConfiguration::Touchpad const& other)
{
    self->merge(*other.self);
}

void miral::InputConfiguration::Keyboard::merge(InputConfiguration::Keyboard const& other)
{
    self->merge(*other.self);
}
