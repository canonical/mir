/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "protobuf_input_converter.h"
#include "mir_protobuf.pb.h"

#include "mir/input/device.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/device_capability.h"

namespace mi = mir::input;
namespace mfd = mir::frontend::detail;
namespace mp = mir::protobuf;

void mfd::pack_protobuf_input_device_info(mp::InputDeviceInfo& device_info, mi::Device const& device)
{
    device_info.set_name(device.name());
    device_info.set_id(device.id());
    device_info.set_unique_id(device.unique_id());
    device_info.set_capabilities(device.capabilities().value());

    auto optional_pointer_conf = device.pointer_configuration();
    if (optional_pointer_conf.is_set())
    {
        auto dev_conf = optional_pointer_conf.consume();
        auto* msg_conf = device_info.mutable_pointer_configuration();
        msg_conf->set_handedness(dev_conf.handedness());
        msg_conf->set_acceleration(dev_conf.acceleration());
        msg_conf->set_acceleration_bias(dev_conf.cursor_acceleration_bias());
        msg_conf->set_horizontal_scroll_scale(dev_conf.horizontal_scroll_scale());
        msg_conf->set_vertical_scroll_scale(dev_conf.vertical_scroll_scale());
    }

    auto optional_touchpad_conf = device.touchpad_configuration();
    if (optional_touchpad_conf.is_set())
    {
        auto dev_conf = optional_touchpad_conf.consume();
        auto* msg_conf = device_info.mutable_touchpad_configuration();
        msg_conf->set_click_modes(dev_conf.click_mode());
        msg_conf->set_scroll_modes(dev_conf.scroll_mode());
        msg_conf->set_button_down_scroll_button(dev_conf.button_down_scroll_button());
        msg_conf->set_tap_to_click(dev_conf.tap_to_click());
        msg_conf->set_middle_mouse_button_emulation(dev_conf.middle_mouse_button_emulation());
        msg_conf->set_disable_with_mouse(dev_conf.disable_with_mouse());
        msg_conf->set_disable_while_typing(dev_conf.disable_while_typing());
    }
}
