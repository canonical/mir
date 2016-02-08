/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#ifndef MIR_TEST_DOUBLES_STUB_INPUT_DEVICE_H_
#define MIR_TEST_DOUBLES_STUB_INPUT_DEVICE_H_

#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/pointer_configuration.h"
#include "mir/input/touchpad_configuration.h"
#include "mir/optional_value.h"

namespace mir
{
namespace test
{
namespace doubles
{

struct StubDevice : input::Device
{
    StubDevice(MirInputDeviceId id, input::DeviceCapabilities caps, std::string const& name, std::string const& unique_id)
        : device_id(id), device_capabilities(caps), device_name(name), device_unique_id(unique_id) {}

    MirInputDeviceId id() const override
    {
        return device_id;
    }
    input::DeviceCapabilities capabilities() const override
    {
        return device_capabilities;
    }
    std::string name() const override
    {
        return device_name;
    }
    std::string unique_id() const override
    {
        return device_unique_id;
    }
    mir::optional_value<input::PointerConfiguration> pointer_configuration() const override
    {
        return {};
    }
    void apply_pointer_configuration(input::PointerConfiguration const&) override
    {
    }

    mir::optional_value<input::TouchpadConfiguration> touchpad_configuration() const override
    {
        return {};
    }
    void apply_touchpad_configuration(input::TouchpadConfiguration const&) override
    {
    }

    MirInputDeviceId device_id;
    input::DeviceCapabilities device_capabilities;
    std::string device_name;
    std::string device_unique_id;
};

}
}
}
#endif

