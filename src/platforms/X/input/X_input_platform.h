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

namespace mir
{
namespace dispatch
{
class ActionQueue;
}
namespace input
{
class InputDevice;

namespace X
{
class XInputDevice;

class XInputPlatform : public mir::input::Platform
{
public:
    explicit XInputPlatform(std::shared_ptr<mir::input::InputDeviceRegistry> const& input_device_registry);
    ~XInputPlatform();

    std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;

private:
    std::shared_ptr<mir::dispatch::ActionQueue> const platform_queue;
    std::shared_ptr<mir::input::InputDeviceRegistry> const registry;
    std::shared_ptr<mir::input::X::XInputDevice> const device;
//    static XInputPlatform* stub_input_platform;
//    static std::vector<std::weak_ptr<mir::input::InputDevice>> device_store;
};

}
}
}

#endif // MIR_INPUT_X_INPUT_PLATFORM_H_
