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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */
#ifndef MIR_INPUT_X_INPUT_PLATFORM_H_
#define MIR_INPUT_X_INPUT_PLATFORM_H_

#include "mir/input/platform.h"
#include <memory>
#include <X11/Xlib.h>

namespace mir
{

namespace dispatch
{
class ActionQueue;
}

namespace input
{

namespace X
{
class XInputDevice;

class XInputPlatform : public input::Platform
{
public:
    explicit XInputPlatform(
        std::shared_ptr<input::InputDeviceRegistry> const& input_device_registry,
        std::shared_ptr<::Display> const& conn);
    ~XInputPlatform() = default;

    std::shared_ptr<dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;

private:
    std::shared_ptr<dispatch::ActionQueue> const platform_queue;
    std::shared_ptr<input::InputDeviceRegistry> const registry;
    std::shared_ptr<XInputDevice> const device;
};

}
}
}

#endif // MIR_INPUT_X_INPUT_PLATFORM_H_
