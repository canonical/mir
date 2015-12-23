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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_DEFAULT_INPUT_DEVICE_HUB_H_
#define MIR_INPUT_DEFAULT_INPUT_DEVICE_HUB_H_

#include "default_event_builder.h"

#include "mir/input/input_device_registry.h"
#include "mir/input/input_sink.h"
#include "mir/input/seat.h"
#include "mir/input/input_device_hub.h"
#include "mir/input/input_device_info.h"

#include "mir_toolkit/event.h"

#include <linux/input.h>
#include <vector>
#include <memory>

namespace mir
{
class ServerActionQueue;
namespace cookie
{
class CookieFactory;
}
namespace dispatch
{
class Dispatchable;
class MultiplexingDispatchable;
class ActionQueue;
}
namespace input
{
class InputSink;
class InputDeviceObserver;
class DefaultDevice;
class Seat;

class DefaultInputDeviceHub : public InputDeviceRegistry, public InputDeviceHub
{
public:
    DefaultInputDeviceHub(std::shared_ptr<Seat> const& seat,
                          std::shared_ptr<dispatch::MultiplexingDispatchable> const& input_multiplexer,
                          std::shared_ptr<ServerActionQueue> const& observer_queue,
                          std::shared_ptr<cookie::CookieFactory> const& cookie_factory);

    // InputDeviceRegistry - calls from mi::Platform
    void add_device(std::shared_ptr<InputDevice> const& device) override;
    void remove_device(std::shared_ptr<InputDevice> const& device) override;

    // InputDeviceHub - calls from server components / shell
    void add_observer(std::shared_ptr<InputDeviceObserver> const&) override;
    void remove_observer(std::weak_ptr<InputDeviceObserver> const&) override;

private:
    void update_spots();
    void add_device_handle(std::shared_ptr<DefaultDevice> const& handle);
    void remove_device_handle(MirInputDeviceId id);
    MirInputDeviceId create_new_device_id();
    std::shared_ptr<Seat> const seat;
    std::shared_ptr<dispatch::MultiplexingDispatchable> const input_dispatchable;
    std::shared_ptr<ServerActionQueue> const observer_queue;
    std::shared_ptr<dispatch::ActionQueue> const device_queue;
    std::shared_ptr<cookie::CookieFactory> const cookie_factory;

    struct RegisteredDevice : public InputSink
    {
    public:
        RegisteredDevice(std::shared_ptr<InputDevice> const& dev,
                         MirInputDeviceId dev_id,
                         std::shared_ptr<dispatch::MultiplexingDispatchable> const& multiplexer,
                         std::shared_ptr<cookie::CookieFactory> const& cookie_factory,
                         std::shared_ptr<DefaultDevice> const& handle);
        void handle_input(MirEvent& event) override;
        mir::geometry::Rectangle bounding_rectangle() const override;
        bool device_matches(std::shared_ptr<InputDevice> const& dev) const;
        void start(std::shared_ptr<Seat> const& seat);
        void stop();
        MirInputDeviceId id();
        std::shared_ptr<Seat> seat;
        const std::shared_ptr<DefaultDevice> handle;
    private:
        MirInputDeviceId device_id;
        DefaultEventBuilder builder;
        std::shared_ptr<InputDevice> const device;
        std::shared_ptr<dispatch::MultiplexingDispatchable> const multiplexer;
    };

    std::vector<std::shared_ptr<DefaultDevice>> handles;
    std::vector<std::unique_ptr<RegisteredDevice>> devices;
    std::vector<std::shared_ptr<InputDeviceObserver>> observers;

    MirInputDeviceId device_id_generator;
};

}
}

#endif
