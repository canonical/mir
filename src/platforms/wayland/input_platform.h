/*
 * Copyright © 2019 Canonical Ltd.
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
 */
#ifndef MIR_INPUT_WAYLAND_INPUT_PLATFORM_H_
#define MIR_INPUT_WAYLAND_INPUT_PLATFORM_H_

#include "mir/input/platform.h"

namespace mir
{
namespace dispatch
{
class ActionQueue;
}
namespace input
{
namespace wayland
{
class InputDevice;

class InputPlatform : public input::Platform
{
public:
    explicit InputPlatform(std::shared_ptr<InputDeviceRegistry> const& input_device_registry);
    ~InputPlatform() = default;

    auto dispatchable() -> std::shared_ptr<dispatch::Dispatchable> override;
    void start() override;
    void stop() override;
    void pause_for_config() override;
    void continue_after_config() override;

private:
    std::shared_ptr<dispatch::ActionQueue> const action_queue;
    std::shared_ptr<InputDeviceRegistry> const registry;
    std::shared_ptr<InputDevice> const keyboard;
    std::shared_ptr<InputDevice> const pointer;
    std::shared_ptr<InputDevice> const touch;
};
}
}
}

#endif // MIR_INPUT_WAYLAND_INPUT_PLATFORM_H_
