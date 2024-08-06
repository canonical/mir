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

#ifndef MIRAL_INPUT_CONFIG_H_
#define MIRAL_INPUT_CONFIG_H_

#include <mir_toolkit/mir_input_device_types.h>

#include <optional>

namespace miral
{
struct MouseInputConfiguration
{
    std::optional<MirPointerHandedness> handedness;
    std::optional<MirPointerAcceleration> acceleration;
    std::optional<double> acceleration_bias;
    std::optional<double> vscroll_speed;
    std::optional<double> hscroll_speed;
};

class TouchpadInputConfiguration
{
public:
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
}

#endif
