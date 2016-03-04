/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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
#include "mir/optional_value.h"

#include <memory>

namespace mir
{
namespace dispatch
{
class ActionQueue;
}
namespace input
{

class InputDevice;

class DefaultDevice : public Device
{
public:
    DefaultDevice(MirInputDeviceId id, std::shared_ptr<dispatch::ActionQueue> const& actions,
                  InputDevice& device);
    MirInputDeviceId id() const override;
    DeviceCapabilities capabilities() const override;
    std::string name() const override;
    std::string unique_id() const override;

    optional_value<PointerConfiguration> pointer_configuration() const override;
    void apply_pointer_configuration(PointerConfiguration const&) override;
    optional_value<TouchpadConfiguration> touchpad_configuration() const override;
    void apply_touchpad_configuration(TouchpadConfiguration const&) override;
private:
    MirInputDeviceId const device_id;
    InputDevice& device;
    InputDeviceInfo const info;
    optional_value<PointerSettings> pointer;
    optional_value<TouchpadSettings> touchpad;
    std::shared_ptr<dispatch::ActionQueue> const actions;
};

}
}

#endif
