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

#ifndef MIRAL_INPUT_DEVICE_CONFIG_H_
#define MIRAL_INPUT_DEVICE_CONFIG_H_

#include <mir/input/input_device_observer.h>
#include <mir/options/option.h>
#include <mir_toolkit/mir_input_device_types.h>

#include <memory>
#include <optional>

namespace mir::input { class Device; }

namespace miral
{
struct MouseInputConfiguration
{
    void apply_to(mir::input::Device& device) const;
    std::optional<MirPointerHandedness> handedness;
    std::optional<MirPointerAcceleration> acceleration;
    std::optional<double> acceleration_bias;
    std::optional<double> vscroll_speed;
    std::optional<double> hscroll_speed;
};

class TouchpadInputConfiguration
{
public:
    void apply_to(mir::input::Device& device) const;
    std::optional<bool> disable_while_typing;
    std::optional<bool> disable_with_external_mouse;
    std::optional<MirPointerAcceleration> acceleration;
    std::optional<double> acceleration_bias;
    std::optional<double> vscroll_speed;
    std::optional<double> hscroll_speed;
    std::optional<MirTouchpadClickMode> click_mode;
    std::optional<MirTouchpadScrollMode> scroll_mode;
    std::optional<bool> tap_to_click;
    std::optional<bool> middle_button_emulation;
};

class InputDeviceConfig : public mir::input::InputDeviceObserver
{
public:
    auto static the_input_configuration(std::shared_ptr<mir::options::Option> const& options)
        -> std::shared_ptr<InputDeviceConfig>;

    void device_added(std::shared_ptr<mir::input::Device> const& device) override;
    void device_changed(std::shared_ptr<mir::input::Device> const&) override {}
    void device_removed(std::shared_ptr<mir::input::Device> const&) override {}
    void changes_complete() override {}

    auto mouse() const -> MouseInputConfiguration;
    auto touchpad() const -> TouchpadInputConfiguration;

    void mouse(MouseInputConfiguration const& val);
    void touchpad(TouchpadInputConfiguration const& val);
private:
    explicit InputDeviceConfig(std::shared_ptr<mir::options::Option> const& options);

    MouseInputConfiguration mouse_config;
    TouchpadInputConfiguration touchpad_config;
};

}

#endif
