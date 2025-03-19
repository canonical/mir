/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_INPUT_CONFIGURATION_H
#define MIRAL_INPUT_CONFIGURATION_H

#include <mir_toolkit/mir_input_device_types.h>

#include <optional>
#include <memory>

namespace mir { class Server; }


namespace miral
{
/** Input configuration.
 * Allow servers to make input configuration changes at runtime
 * \remark Since MirAL 5.1
 */
class InputConfiguration
{
public:
    InputConfiguration();
    ~InputConfiguration();
    void operator()(mir::Server& server);

    class Mouse;
    class Touchpad;
    class Keyboard;

    auto mouse() -> Mouse;
    void mouse(Mouse const& val);
    auto touchpad() -> Touchpad;
    void touchpad(Touchpad const& val);
    auto keyboard() -> Keyboard;
    void keyboard(Keyboard const& val);

private:
    class Self;
    std::shared_ptr<Self> self;
};

/** Input configuration for mouse pointer devices
 * \remark Since MirAL 5.1
 */
class InputConfiguration::Mouse
{
public:
    Mouse();
    ~Mouse();

    Mouse(Mouse const& that);
    auto operator=(Mouse that) -> Mouse&;
    auto operator==(Mouse const& that) const -> bool;

    auto handedness() const -> std::optional<MirPointerHandedness>;
    auto acceleration() const -> std::optional<MirPointerAcceleration>;
    auto acceleration_bias() const -> std::optional<double>;
    auto vscroll_speed() const -> std::optional<double>;
    auto hscroll_speed() const -> std::optional<double>;

    void handedness(std::optional<MirPointerHandedness>const& val);
    void acceleration(std::optional<MirPointerAcceleration>const& val);
    /// \note val will be clamped to the range [-1.0, 1.0]
    void acceleration_bias(std::optional<double>const& val);
    void vscroll_speed(std::optional<double>const& val);
    void hscroll_speed(std::optional<double>const& val);

    /// Copies the corresponding value from `other` for all unset fields.
    /// \remark Since MirAL 5.3
    void merge(InputConfiguration::Mouse const& other);

private:
    friend class InputConfiguration::Self;
    class Self;
    std::unique_ptr<Self> self;
};

/** Input configuration for touchpad devices
 * \remark Since MirAL 5.1
 */
class InputConfiguration::Touchpad
{
public:
    Touchpad();
    ~Touchpad();

    Touchpad(Touchpad const& that);
    auto operator=(Touchpad that) -> Touchpad&;
    auto operator==(Touchpad const& that) const -> bool;

    auto disable_while_typing() const -> std::optional<bool>;
    auto disable_with_external_mouse() const -> std::optional<bool>;
    auto acceleration() const -> std::optional<MirPointerAcceleration>;
    auto acceleration_bias() const -> std::optional<double>;
    auto vscroll_speed() const -> std::optional<double>;
    auto hscroll_speed() const -> std::optional<double>;
    auto click_mode() const -> std::optional<MirTouchpadClickMode>;
    auto scroll_mode() const -> std::optional<MirTouchpadScrollMode>;
    auto tap_to_click() const -> std::optional<bool>;
    /// \remark Since MirAL 5.3
    auto middle_mouse_button_emulation() const -> std::optional<bool>;

    void disable_while_typing(std::optional<bool>const& val);
    void disable_with_external_mouse(std::optional<bool>const& val);
    void acceleration(std::optional<MirPointerAcceleration>const& val);
    /// \note val will be clamped to the range [-1.0, 1.0]
    void acceleration_bias(std::optional<double>const& val);
    void vscroll_speed(std::optional<double>const& val);
    void hscroll_speed(std::optional<double>const& val);
    void click_mode(std::optional<MirTouchpadClickMode>const& val);
    void scroll_mode(std::optional<MirTouchpadScrollMode>const& val);
    void tap_to_click(std::optional<bool>const& val);
    /// \remark Since MirAL 5.3
    void middle_mouse_button_emulation(std::optional<bool>const& val);

    /// Copies the corresponding value from `other` for all unset fields.
    /// \remark Since MirAL 5.3
    void merge(InputConfiguration::Touchpad const& other);

private:
    friend class InputConfiguration::Self;
    class Self;
    std::shared_ptr<Self> self;
};

/** Input configuration for keyboard devices
 * \remark Since MirAL 5.3
 */
class InputConfiguration::Keyboard
{
public:
    Keyboard();
    ~Keyboard();

    Keyboard(Keyboard const& that);
    auto operator=(Keyboard that) -> Keyboard&;
    auto operator==(Keyboard const& that) const -> bool;

    void set_repeat_rate(int new_rate);
    void set_repeat_delay(int new_delay);

    void merge(InputConfiguration::Keyboard const& other);

private:
    friend class InputConfiguration::Self;
    class Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_INPUT_CONFIGURATION_H
