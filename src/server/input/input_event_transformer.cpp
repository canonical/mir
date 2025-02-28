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

#include "mir/input/device_capability.h"
#include "mir/input/event_builder.h"
#include "mir/input/input_device_registry.h"
#include "mir/input/input_sink.h"
#include "mir/input/virtual_input_device.h"
#include "mir/main_loop.h"

#include <cassert>
#include <memory>
#include <mutex>

namespace mi = mir::input;

mi::InputEventTransformer::InputEventTransformer(
    std::shared_ptr<InputDeviceRegistry> const& input_device_registry, std::shared_ptr<MainLoop> const& main_loop) :
    virtual_pointer{
        std::make_shared<mir::input::VirtualInputDevice>("mousekey-pointer", mir::input::DeviceCapability::pointer)},
    input_device_registry{input_device_registry},
    main_loop{main_loop}
{
    input_device_registry->add_device(virtual_pointer);
}

mir::input::InputEventTransformer::~InputEventTransformer()
{
    if(virtual_pointer)
        input_device_registry->remove_device(virtual_pointer);
}

bool mi::InputEventTransformer::handle(MirEvent const& event)
{
    std::lock_guard lock{mutex};

    auto handled = false;
    virtual_pointer->if_started_then(
        [this, &event, &handled](auto* sink, auto* builder)
        {
            auto const dispatcher = [main_loop = this->main_loop, sink](auto event)
            {
                main_loop->spawn(
                    [sink, event]
                    {
                        sink->handle_input(event);
                    });
            };

            for (auto it = input_transformers.begin(); it != input_transformers.end();)
            {
                auto const& t = it->lock();
                if (!t)
                {
                    it = input_transformers.erase(it);
                    continue;
                }

                if (t->transform_input_event(dispatcher, builder, event))
                {
                    handled = true;
                    break;
                }
                ++it;
            }
        });

    return handled;
}

void mi::InputEventTransformer::append(std::weak_ptr<mi::InputEventTransformer::Transformer> const& transformer)
{
    std::lock_guard lock{mutex};
    input_transformers.push_back(transformer);
}
