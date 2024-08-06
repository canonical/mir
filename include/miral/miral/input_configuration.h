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
/** Allow servers to make input configuration changes at runtime
 */
class InputConfiguration
{
public:
    InputConfiguration();
    ~InputConfiguration();
    void operator()(mir::Server& server);

    class Mouse;
    class Touchpad;

    auto mouse() -> Mouse;
    void mouse(Mouse);
    auto touchpad() -> Touchpad;
    void touchpad(Touchpad);

private:
    class Self;
    std::shared_ptr<Self> self;
};

/*
  --mouse-handedness arg                Select mouse laterality: [right, left]
  --mouse-cursor-acceleration arg       Select acceleration profile for mice
                                        and trackballs [none, adaptive]
  --mouse-cursor-acceleration-bias arg  Set the pointer acceleration speed of
                                        mice within a range of [-1.0, 1.0]
  --mouse-scroll-speed arg              Scales mouse scroll. Use negative
                                        values for natural scrolling
  --mouse-horizontal-scroll-speed-override arg
                                        Scales mouse horizontal scroll. Use
                                        negative values for natural scrolling
  --mouse-vertical-scroll-speed-override arg
                                        Scales mouse vertical scroll. Use
                                        negative values for natural scrolling
 */
class InputConfiguration::Mouse
{
public:
    Mouse();
    ~Mouse();

    Mouse(Mouse const& that);
    auto operator=(Mouse that) -> Mouse&;

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

private:
    class Self;
    std::unique_ptr<Self> self;
};

/*
  --touchpad-disable-while-typing arg   Disable touchpad while typing on
                                        keyboard configuration [true, false]
  --touchpad-disable-with-external-mouse arg
                                        Disable touchpad if an external pointer
                                        device is plugged in [true, false]
  --touchpad-tap-to-click arg           Enable or disable tap-to-click on this
                                        device, with 1, 2, 3 finger tap mapping
                                        to left, right, middle click,
                                        respectively [true, false]
  --touchpad-cursor-acceleration arg    Select acceleration profile for
                                        touchpads [none, adaptive]
  --touchpad-cursor-acceleration-bias arg
                                        Set the pointer acceleration speed of
                                        touchpads within a range of [-1.0, 1.0]
  --touchpad-scroll-speed arg           Scales touchpad scroll. Use negative
                                        values for natural scrolling
  --touchpad-horizontal-scroll-speed-override arg
                                        Scales touchpad horizontal scroll. Use
                                        negative values for natural scrolling
  --touchpad-vertical-scroll-speed-override arg
                                        Scales touchpad vertical scroll. Use
                                        negative values for natural scrolling
  --touchpad-scroll-mode arg            Select scroll mode for touchpads:
                                        [edge, two-finger, button-down]
  --touchpad-click-mode arg             Select click mode for touchpads: [none,
                                        area, clickfinger]
  --touchpad-middle-mouse-button-emulation arg
                                        Converts a simultaneous left and right
                                        button click into a middle button
 */
class InputConfiguration::Touchpad
{
public:
    Touchpad();
    ~Touchpad();

    Touchpad(Touchpad const& that);
    auto operator=(Touchpad that) -> Touchpad&;

    auto disable_while_typing() const -> std::optional<bool>;
    auto disable_with_external_mouse() const -> std::optional<bool>;
    auto acceleration() const -> std::optional<MirPointerAcceleration>;
    auto acceleration_bias() const -> std::optional<double>;
    auto vscroll_speed() const -> std::optional<double>;
    auto hscroll_speed() const -> std::optional<double>;
    auto click_mode() const -> std::optional<MirTouchpadClickMode>;
    auto scroll_mode() const -> std::optional<MirTouchpadScrollMode>;
    auto tap_to_click() const -> std::optional<bool>;

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

private:
    class Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_INPUT_CONFIGURATION_H
