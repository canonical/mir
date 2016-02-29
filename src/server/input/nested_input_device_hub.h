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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_NESTED_INPUT_DEVICE_HUB_H_
#define MIR_INPUT_NESTED_INPUT_DEVICE_HUB_H_

#include "mir_toolkit/mir_connection.h"
#include "mir/input/input_device_hub.h"
#include <vector>
#include <mutex>

struct MirConnection;

namespace mir
{
class ServerActionQueue;
namespace frontend
{
class EventSink;
}
namespace input
{

using UniqueInputConfig = std::unique_ptr<MirInputConfig, void(*)(MirInputConfig const*)>;

class NestedInputDeviceHub : public InputDeviceHub
{
public:
    NestedInputDeviceHub(std::shared_ptr<MirConnection> const& connection,
                         std::shared_ptr<frontend::EventSink> const& sink,
                         std::shared_ptr<mir::ServerActionQueue> const& observer_queue);
    void add_observer(std::shared_ptr<InputDeviceObserver> const&) override;
    void remove_observer(std::weak_ptr<InputDeviceObserver> const&) override;
    void for_each_input_device(std::function<void(std::shared_ptr<Device>const& device)> const& callback) override;

private:
    void update_devices();
    std::shared_ptr<MirConnection> const connection;
    std::shared_ptr<frontend::EventSink> const sink;
    std::shared_ptr<mir::ServerActionQueue> const observer_queue;
    std::vector<std::shared_ptr<InputDeviceObserver>> observers;
    std::mutex devices_guard;
    std::vector<std::shared_ptr<Device>> devices;
    UniqueInputConfig config;
};

}
}

#endif
