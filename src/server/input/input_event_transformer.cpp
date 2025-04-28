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

#include "mir/input/input_event_transformer.h"

#include "mir/events/event_builders.h"
#include "mir/input/device.h"
#include "mir/input/device_capability.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_sink.h"
#include "mir/input/seat.h"
#include "mir/input/virtual_input_device.h"
#include "mir/main_loop.h"

#include <algorithm>

namespace mi = mir::input;

mi::InputEventTransformer::InputEventTransformer(std::shared_ptr<Seat> const& seat) :
    seat{seat},
    virtual_device{
        std::make_shared<mir::input::VirtualInputDevice>("mousekey-pointer", mir::input::DeviceCapability::pointer)}
{
}

mir::input::InputEventTransformer::~InputEventTransformer() = default;

bool mir::input::InputEventTransformer::handle(MirEvent const& event)
{
    auto handled = false;
    virtual_device->if_started_then(
        [this, &event, &handled](auto*, auto* builder)
        {
            handled = transform(
                event,
                builder,
                [this](auto event) { seat->dispatch_event(event); },
                weak_device.lock()->id());
        });
    return handled;
}

bool mi::InputEventTransformer::transform(
    MirEvent const& event, EventBuilder* builder, EventDispatcher const& dispatcher, MirInputDeviceId virtual_device_id)
{
    std::lock_guard lock{mutex};

    for (auto it = input_transformers.begin(); it != input_transformers.end();)
    {
        if (it->expired())
        {
            it = input_transformers.erase(it);
        }
        else
        {
            ++it;
        }
    }

    input_transformers.shrink_to_fit();

    auto handled = false;

    for (auto it : input_transformers)
    {
        auto const& t = it.lock();

        if (t->transform_input_event(dispatcher, builder, event, virtual_device_id))
        {
            handled = true;
            break;
        }
    }

    return handled;
}

bool mi::InputEventTransformer::append(std::weak_ptr<mi::InputEventTransformer::Transformer> const& transformer)
{
    std::lock_guard lock{mutex};

    auto const duplicate_iter = std::ranges::find(
            input_transformers,
            transformer.lock().get(),
            [](auto const& other_transformer)
            {
                return other_transformer.lock().get();
            });

    if (duplicate_iter != input_transformers.end())
        return false;

    input_transformers.push_back(transformer);

    if(weak_device.expired())
        weak_device = input_device_registry->add_device(virtual_device);

    return true;
}

bool mi::InputEventTransformer::remove(std::shared_ptr<mi::InputEventTransformer::Transformer> const& transformer)
{
    std::lock_guard lock{mutex};
    auto [remove_start, remove_end] = std::ranges::remove(
        input_transformers,
        transformer,
        [](auto const& list_element)
        {
            return list_element.lock();
        });

    if (remove_start == input_transformers.end())
        return false;

    input_transformers.erase(remove_start, remove_end);

    if(input_transformers.empty())
    {
        input_device_registry->remove_device(virtual_device);
        weak_device.reset();
    }

    return true;
}

void mir::input::InputEventTransformer::init(std::shared_ptr<InputDeviceRegistry> const& input_device_registry)
{
    this->input_device_registry = input_device_registry;
}
