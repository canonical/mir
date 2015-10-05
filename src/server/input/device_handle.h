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
#include "mir/module_deleter.h"

#include <memory>

namespace mir
{
namespace input
{

class DefaultDevice : public Device
{
public:
    DefaultDevice(MirInputDeviceId id, InputDeviceInfo const& info);
    MirInputDeviceId id() const override;
    DeviceCapabilities capabilities() const override;
    std::string name() const override;
    std::string unique_id() const override;
private:
    MirInputDeviceId device_id;
    InputDeviceInfo info;
};

}
}

#endif
