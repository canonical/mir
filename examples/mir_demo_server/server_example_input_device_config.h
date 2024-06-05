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

#ifndef MIR_EXAMPLES_INPUT_DEVICE_CONFIG_H_
#define MIR_EXAMPLES_INPUT_DEVICE_CONFIG_H_

#include "mir/input/input_device_observer.h"
#include "mir_toolkit/mir_input_device_types.h"
#include <optional>

namespace mir
{
class Server;

namespace input
{
class Device;
}

namespace examples
{
void add_input_device_configuration_options_to(Server& server);

class InputDeviceConfig : public mir::input::InputDeviceObserver
{
public:
    InputDeviceConfig(std::optional<bool> disable_while_typing,
                      std::optional<bool> tap_to_click,
                      std::optional<MirPointerAcceleration> mouse_profile,
                      std::optional<double> mouse_cursor_acceleration_bias,
                      std::optional<double> mouse_scroll_speed_scale,
                      std::optional<double> touchpad_cursor_acceleration_bias,
                      std::optional<double> touchpad_scroll_speed_scale,
                      std::optional<MirTouchpadClickMode> click_mode,
                      std::optional<MirTouchpadScrollMode> scroll_mode);
    void device_added(std::shared_ptr<input::Device> const& device) override;
    void device_changed(std::shared_ptr<input::Device> const&) override {}
    void device_removed(std::shared_ptr<input::Device> const&) override {}
    void changes_complete() override {}
private:
    std::optional<bool> const disable_while_typing;
    std::optional<MirPointerAcceleration> const mouse_profile;
    std::optional<double> const mouse_cursor_acceleration_bias;
    std::optional<double> const mouse_scroll_speed_scale;
    std::optional<double> const touchpad_cursor_acceleration_bias;
    std::optional<double> const touchpad_scroll_speed_scale;
    std::optional<MirTouchpadClickMode> const click_mode;
    std::optional<MirTouchpadScrollMode> const scroll_mode;
    std::optional<bool> const tap_to_click;
};

}
}

#endif
