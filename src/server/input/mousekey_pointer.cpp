/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/input/mousekey_pointer.h"
#include "mir/input/device.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_event_transformer.h"
#include "mir/input/input_sink.h"
#include "mir/input/virtual_input_device.h"
#include "mir/main_loop.h"

mir::input::MousekeyPointer::MousekeyPointer(
    std::shared_ptr<MainLoop> main_loop, std::shared_ptr<input::InputEventTransformer> iet) :
    main_loop{std::move(main_loop)},
    iet{std::move(iet)},
    virtual_device{

        std::make_shared<mir::input::VirtualInputDevice>("mousekey-pointer", mir::input::DeviceCapability::pointer)}

{
}

bool mir::input::MousekeyPointer::handle(MirEvent const& event)
{
    auto handled = false;
    virtual_device->if_started_then(
        [this, &event, &handled](auto* sink, auto* builder)
        {
            handled = iet->transform(
                event,
                builder,
                [this, sink](auto event) { main_loop->spawn([sink, event] { sink->handle_input(event); }); },
                state.lock()->device_id);
        });
    return handled;
}

void mir::input::MousekeyPointer::add_to_registry(std::shared_ptr<InputDeviceRegistry> const& registry)
{
    auto mutable_state = state.lock();
    if(!mutable_state->weak_device.expired()) return; // Already registered

    mutable_state->weak_device = registry->add_device(virtual_device);
    mutable_state->device_id = mutable_state->weak_device.lock()->id();
}

void mir::input::MousekeyPointer::remove_from_registry(std::shared_ptr<InputDeviceRegistry> const& registry)
{
    auto mutable_state = state.lock();

    if(mutable_state->weak_device.expired()) return; // Already unregistered
    registry->remove_device(virtual_device);
    mutable_state->weak_device.reset();
}
