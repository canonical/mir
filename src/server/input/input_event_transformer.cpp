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
#include "mir/input/input_device_registry.h"
#include "mir/input/input_sink.h"
#include "mir/input/virtual_input_device.h"
#include "mir/main_loop.h"
#include "mir/log.h"

#include <algorithm>
#include <boost/throw_exception.hpp>
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

            for (auto transformer : input_transformers)
            {
                if (transformer->transform_input_event(dispatcher, builder, event))
                {
                    handled = true;
                    break;
                }
            }
        });

    return handled;
}

auto mi::InputEventTransformer::append(std::shared_ptr<mi::InputEventTransformer::Transformer> const& transformer)
    -> Registration
{
    std::lock_guard lock{mutex};

    auto const duplicate_iter = std::ranges::find(
            input_transformers,
            transformer.get(),
            [](auto const& other_transformer)
            {
                return other_transformer.get();
            });

    if (duplicate_iter != input_transformers.end())
        BOOST_THROW_EXCEPTION(std::runtime_error("Transformer already registered"));

    input_transformers.push_back(transformer);

    return Registration(this, transformer);
}

void mi::InputEventTransformer::remove(std::shared_ptr<mi::InputEventTransformer::Transformer> const& transformer)
{
    std::lock_guard lock{mutex};

    auto [remove_start, remove_end] = std::ranges::remove(input_transformers, transformer);
    input_transformers.erase(remove_start, remove_end);
}

mir::input::InputEventTransformer::Registration::Registration(
    InputEventTransformer* event_transformer,
    std::shared_ptr<mir::input::InputEventTransformer::Transformer> transformer) :
    unregister(
        [event_transformer, transformer]()
        {
            event_transformer->remove(transformer);
        })
{
}

mir::input::InputEventTransformer::Registration::~Registration()
{
    unregister();
}

void mir::input::InputEventTransformer::Registration::swap(Registration& other) noexcept
{
    unregister.swap(other.unregister);
}

mir::input::InputEventTransformer::Registration::Registration(Registration&& other):
    Registration()
{
    other.swap(*this);
}

mir::input::InputEventTransformer::Registration::Registration() :
    unregister{[]
               {
               }}
{
}
