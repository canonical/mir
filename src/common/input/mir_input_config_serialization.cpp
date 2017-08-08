/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/mir_input_config_serialization.h"
#include "mir/input/mir_input_config.h"
#include "mir/input/mir_pointer_config.h"
#include "mir/input/mir_touchpad_config.h"
#include "mir/input/mir_touchscreen_config.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir_input_config.capnp.h"

#include <capnp/message.h>
#include <capnp/serialize.h>

namespace mi = mir::input;
namespace mc = mir::capnp;

std::string mi::serialize_input_config(MirInputConfig const& config)
{
    ::capnp::MallocMessageBuilder message;
    mc::InputConfig::Builder builder = message.initRoot<mc::InputConfig>();
    auto list_builder = builder.initDevices(config.size());
    auto device_iterator = list_builder.begin();

    config.for_each(
        [&](MirInputDevice const& conf)
        {
            auto device = *device_iterator++;
            device.initId().setId(conf.id());
            device.setCapabilities(conf.capabilities().value());
            device.setName(conf.name());
            device.setUniqueId(conf.unique_id());

            if (conf.has_pointer_config())
            {
                auto ptr_conf = conf.pointer_config();
                auto ptr_builder = device.initPointerConfig();
                ptr_builder.setHandedness(ptr_conf.handedness() == mir_pointer_handedness_right ?
                                          mc::PointerConfig::Handedness::RIGHT :
                                          mc::PointerConfig::Handedness::LEFT);
                ptr_builder.setAcceleration(ptr_conf.acceleration() == mir_pointer_acceleration_adaptive ?
                                            mc::PointerConfig::Acceleration::ADAPTIVE :
                                            mc::PointerConfig::Acceleration::NONE
                                           );
                ptr_builder.setCursorAccelerationBias(ptr_conf.cursor_acceleration_bias());
                ptr_builder.setHorizontalScrollScale(ptr_conf.horizontal_scroll_scale());
                ptr_builder.setVerticalScrollScale(ptr_conf.vertical_scroll_scale());
            }

            if (conf.has_touchpad_config())
            {
                auto tpd_conf = conf.touchpad_config();
                auto tpd_builder = device.initTouchpadConfig();

                tpd_builder.setClickMode(tpd_conf.click_mode());
                tpd_builder.setScrollMode(tpd_conf.scroll_mode());
                tpd_builder.setButtonDownScrollButton(tpd_conf.button_down_scroll_button());
                tpd_builder.setTapToClick(tpd_conf.tap_to_click());
                tpd_builder.setMiddleMouseButtonEmulation(tpd_conf.middle_mouse_button_emulation());
                tpd_builder.setDisableWithMouse(tpd_conf.disable_with_mouse());
                tpd_builder.setDisableWhileTyping(tpd_conf.disable_while_typing());
            }

            if (conf.has_touchscreen_config())
            {
                auto ts_conf = conf.touchscreen_config();
                auto ts_builder = device.initTouchscreenConfig();

                ts_builder.setOutputId(ts_conf.output_id());
                ts_builder.setMappingMode(ts_conf.mapping_mode() == mir_touchscreen_mapping_mode_to_output?
                                          mc::TouchscreenConfig::MappingMode::TO_OUTPUT :
                                          mc::TouchscreenConfig::MappingMode::TO_DISPLAY_WALL);
            }

            if (conf.has_keyboard_config())
            {
                auto kbd_conf = conf.keyboard_config();
                auto kbd_builder = device.initKeyboardConfig();
                auto keymap_builder = kbd_builder.initKeymap();
                auto keymap = kbd_conf.device_keymap();

                keymap_builder.setModel(keymap.model);
                keymap_builder.setLayout(keymap.layout);
                keymap_builder.setVariant(keymap.variant);
                keymap_builder.setOptions(keymap.options);
            }
        });

    auto flat = ::capnp::messageToFlatArray(message);

    return {reinterpret_cast<char*>(flat.asBytes().begin()), flat.asBytes().size()};
}

MirInputConfig mi::deserialize_input_config(std::string const& buffer)
{
    ::capnp::MallocMessageBuilder message;
    kj::ArrayPtr<::capnp::word const> words(reinterpret_cast<::capnp::word const*>(
        buffer.data()), buffer.size() / sizeof(::capnp::word));
    ::capnp::initMessageBuilderFromFlatArrayCopy(words, message);
    mc::InputConfig::Reader conf_reader = message.getRoot<mc::InputConfig>();

    MirInputConfig ret;

    for (auto const& device_config : conf_reader.getDevices())
    {
        MirInputDevice conf(device_config.getId().getId(),
                            static_cast<mi::DeviceCapabilities>(device_config.getCapabilities()),
                            device_config.getName(),
                            device_config.getUniqueId());
        if (device_config.hasTouchpadConfig())
        {
            auto tpd_conf = device_config.getTouchpadConfig();
            conf.set_touchpad_config(
                MirTouchpadConfig{
                    tpd_conf.getClickMode(),
                    tpd_conf.getScrollMode(),
                    tpd_conf.getButtonDownScrollButton(),
                    tpd_conf.getTapToClick(),
                    tpd_conf.getDisableWhileTyping(),
                    tpd_conf.getDisableWithMouse(),
                    tpd_conf.getMiddleMouseButtonEmulation(),
                    });
        }

        if (device_config.hasPointerConfig())
        {
            auto pointer_conf = device_config.getPointerConfig();
            conf.set_pointer_config(
                MirPointerConfig{
                    pointer_conf.getHandedness()==mc::PointerConfig::Handedness::RIGHT ?
                    mir_pointer_handedness_right :
                    mir_pointer_handedness_left,
                    pointer_conf.getAcceleration()==mc::PointerConfig::Acceleration::ADAPTIVE ?
                    mir_pointer_acceleration_adaptive :
                    mir_pointer_acceleration_none,
                    pointer_conf.getCursorAccelerationBias(),
                    pointer_conf.getHorizontalScrollScale(),
                    pointer_conf.getVerticalScrollScale()
                    });
        }

        if (device_config.hasKeyboardConfig())
        {
            auto keyboard_conf = device_config.getKeyboardConfig();
            auto keymap_reader = keyboard_conf.getKeymap();
            conf.set_keyboard_config(
                MirKeyboardConfig{
                    Keymap{
                        keymap_reader.getModel(),
                        keymap_reader.getLayout(),
                        keymap_reader.getVariant(),
                        keymap_reader.getOptions()
                    }
                });
        }

        if (device_config.hasTouchscreenConfig())
        {
            auto touchscreen_conf = device_config.getTouchscreenConfig();
            conf.set_touchscreen_config(
                MirTouchscreenConfig{
                    touchscreen_conf.getOutputId(),
                    touchscreen_conf.getMappingMode() == mc::TouchscreenConfig::MappingMode::TO_OUTPUT ?
                    mir_touchscreen_mapping_mode_to_output :
                    mir_touchscreen_mapping_mode_to_display_wall
                });
        }
        ret.add_device_config(std::move(conf));
    }

    return ret;
}
