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
 * Authored by:
 *   Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_DEFAULT_DEVICE_H_
#define MIR_INPUT_DEFAULT_DEVICE_H_

#include "mir_toolkit/event.h"
#include "mir/input/device.h"
#include "mir/input/input_device_info.h"
#include "mir/input/pointer_settings.h"
#include "mir/input/touchpad_settings.h"
#include "mir/input/touchscreen_settings.h"
#include "mir/input/mir_keyboard_config.h"
#include "mir/optional_value.h"

#include <memory>
#include <functional>
#include <mutex>

namespace mir
{
namespace dispatch
{
class ActionQueue;
}
namespace input
{

class KeyMapper;
class InputDevice;
class DefaultInputDeviceHub;

class DefaultDevice : public Device
{
public:
    DefaultDevice(MirInputDeviceId id,
                  std::shared_ptr<dispatch::ActionQueue> const& actions,
                  InputDevice& device,
                  std::shared_ptr<KeyMapper> const& key_mapper);
    DefaultDevice(MirInputDeviceId id,
                  std::shared_ptr<dispatch::ActionQueue> const& actions,
                  InputDevice& device,
                  std::shared_ptr<KeyMapper> const& key_mapper,
                  std::function<void(Device*)> const& change_callback);
    DefaultDevice(MirInputDevice const& config,
                  std::shared_ptr<dispatch::ActionQueue> const& actions,
                  InputDevice& device,
                  std::shared_ptr<KeyMapper> const& key_mapper,
                  std::function<void(Device*)> const& change_callback);
    MirInputDeviceId id() const override;
    DeviceCapabilities capabilities() const override;
    std::string name() const override;
    std::string unique_id() const override;

    optional_value<MirPointerConfig> pointer_configuration() const override;
    void apply_pointer_configuration(MirPointerConfig const&) override;
    optional_value<MirTouchpadConfig> touchpad_configuration() const override;
    void apply_touchpad_configuration(MirTouchpadConfig const&) override;
    optional_value<MirKeyboardConfig> keyboard_configuration() const override;
    void apply_keyboard_configuration(MirKeyboardConfig const&) override;
    optional_value<MirTouchscreenConfig> touchscreen_configuration() const override;
    void apply_touchscreen_configuration(MirTouchscreenConfig const&) override;

    MirInputDevice config() const;
    void disable_queue();
private:
    void set_pointer_configuration(MirPointerConfig const&);
    void set_touchpad_configuration(MirTouchpadConfig const&);
    void set_keyboard_configuration(MirKeyboardConfig const&);
    void set_touchscreen_configuration(MirTouchscreenConfig const&);

    void wake_hub_for_device_change();
    MirInputDeviceId const device_id;
    InputDevice& device;
    InputDeviceInfo const info;
    optional_value<PointerSettings> pointer;
    optional_value<TouchpadSettings> touchpad;
    optional_value<MirKeyboardConfig> keyboard;
    optional_value<TouchscreenSettings> touchscreen;
    std::shared_ptr<dispatch::ActionQueue> actions;
    std::shared_ptr<KeyMapper> const key_mapper;
    std::function<void(Device*)> device_changed_callback;
    std::mutex mutable config_mutex;
};

}
}

#endif
