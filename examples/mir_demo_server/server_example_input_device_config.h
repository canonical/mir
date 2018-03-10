/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored By: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_EXAMPLES_INPUT_DEVICE_CONFIG_H_
#define MIR_EXAMPLES_INPUT_DEVICE_CONFIG_H_

#include "mir/input/input_device_observer.h"
#include "mir_toolkit/mir_input_device.h"

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
    InputDeviceConfig(bool disable_while_typing,
                      MirPointerAcceleration mouse_profile,
                      double mouse_cursor_acceleration_bias,
                      double mouse_scroll_speed_scale,
                      double touchpad_cursor_acceleration_bias,
                      double touchpad_scroll_speed_scale,
                      MirTouchpadClickModes click_mode,
                      MirTouchpadScrollModes scroll_mode);
    void device_added(std::shared_ptr<input::Device> const& device) override;
    void device_changed(std::shared_ptr<input::Device> const&) override {};
    void device_removed(std::shared_ptr<input::Device> const&) override {};
    void changes_complete() override {}
private:
    bool disable_while_typing;
    MirPointerAcceleration mouse_profile;
    double mouse_cursor_acceleration_bias;
    double mouse_scroll_speed_scale;
    double touchpad_cursor_acceleration_bias;
    double touchpad_scroll_speed_scale;
    MirTouchpadClickModes click_mode;
    MirTouchpadScrollModes scroll_mode;
};

}
}

#endif
