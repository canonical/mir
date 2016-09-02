/*
 * Copyright © 2016 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_NESTED_INPUT_PLATFORM_H_
#define MIR_GRAPHICS_NESTED_INPUT_PLATFORM_H_

#include "host_connection.h"
#include "mir/input/platform.h"
#include "mir/dispatch/dispatchable.h"
#include "mir/dispatch/action_queue.h"

#include <memory>
#include <unordered_map>
#include <mutex>

namespace mir
{
namespace input
{
class InputDevice;
class InputDeviceRegistry;
class InputReport;
}
namespace graphics
{
namespace nested
{
class HostConnection;
class InputPlatform : public input::Platform
{
public:
    InputPlatform(std::shared_ptr<HostConnection> const& connection,
                  std::shared_ptr<input::InputDeviceRegistry> const& input_device_registry,
                  std::shared_ptr<input::InputReport> const& report);
    std::shared_ptr<mir::dispatch::Dispatchable> dispatchable() override;
    void start() override;
    void stop() override;

private:
    void update_devices();
    void update_devices_locked();
    struct InputDevice;
    std::shared_ptr<HostConnection> const connection;
    std::shared_ptr<input::InputDeviceRegistry> const input_device_registry;
    std::shared_ptr<input::InputReport> const report;
    std::shared_ptr<dispatch::ActionQueue> const action_queue;
    std::mutex devices_guard;
    UniqueInputConfig input_config;

    using EventUPtr = std::unique_ptr<MirEvent, void(*)(MirEvent*)>;
    std::unordered_map<MirInputDeviceId, std::shared_ptr<InputDevice>> devices;
    std::unordered_map<MirInputDeviceId, std::vector<std::pair<EventUPtr, mir::geometry::Rectangle>>>
        unknown_device_events;
};
}
}
}

#endif
