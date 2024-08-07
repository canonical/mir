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

#include "input_config.h"

#include <mir/server.h>
#include <mir/input/input_device_hub.h>

#include <algorithm>

class miral::InputConfiguration::Mouse::Self : public MouseInputConfiguration
{
};

class miral::InputConfiguration::Touchpad::Self : public TouchpadInputConfiguration
{
};

class miral::InputConfiguration::Self
{
public:
    std::shared_ptr<mir::input::InputDeviceHub> input_device_hub{};

    Mouse mouse;
    Touchpad touchpad;

    void apply(Mouse const& m)
    {
        if (input_device_hub)
        {
            input_device_hub->for_each_mutable_input_device([&m](auto& input_device)
            {
                m.self->apply_to(input_device);
            });
        }

        mouse = m;
    }

    void apply(Touchpad const& t)
    {
        if (input_device_hub)
        {
            input_device_hub->for_each_mutable_input_device([&t](auto& input_device)
            {
                t.self->apply_to(input_device);
            });
        }

        touchpad = t;
    }
};

miral::InputConfiguration::InputConfiguration() :
    self{std::make_shared<Self>()}
{
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
    });
}

auto miral::InputConfiguration::mouse() -> Mouse
{
    return self->mouse;
}

auto miral::InputConfiguration::touchpad() -> Touchpad
{
    return self->touchpad;
}

void miral::InputConfiguration::touchpad(Touchpad const& t)
{
    self->apply(t);
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
    return self->acceleration_bias;
}

auto miral::InputConfiguration::Mouse::hscroll_speed() const -> std::optional<double>
{
    return self->acceleration_bias;
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
