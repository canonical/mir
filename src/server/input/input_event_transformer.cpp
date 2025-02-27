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

#include <memory>
#include <mutex>

namespace mi = mir::input;

mi::InputEventTransformer::InputEventTransformer(
    std::shared_ptr<InputDeviceRegistry> const& device_registry, std::shared_ptr<MainLoop> const& main_loop) :
    input_device_registry{device_registry},
    main_loop{main_loop}
{
}

mir::input::InputEventTransformer::~InputEventTransformer()
{
    if(virtual_pointer)
        input_device_registry->remove_device(virtual_pointer);
}

bool mi::InputEventTransformer::handle(MirEvent const& event)
{
    std::lock_guard lock{mutex};

    if(!virtual_pointer)
        return false;

    auto handled = false;
    virtual_pointer->if_started_then(
        [this, &event, &handled](mir::input::InputSink* sink, mir::input::EventBuilder* builder)
        {
            if (!dispatcher)
                dispatcher = std::make_shared<EventDispatcher>(main_loop, sink, builder);

            for (auto it = input_transformers.begin(); it != input_transformers.end();)
            {
                auto const& t = it->lock();
                if (!t)
                {
                    it = input_transformers.erase(it);
                    continue;
                }

                if (t->transform_input_event(dispatcher, event))
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
    lazily_init_virtual_input_device();
    input_transformers.push_back(transformer);
}

void mi::InputEventTransformer::lazily_init_virtual_input_device()
{
    if (input_transformers.empty())
    {
        virtual_pointer =
            std::make_shared<input::VirtualInputDevice>("mousekey-pointer", mir::input::DeviceCapability::pointer);
        input_device_registry->add_device(virtual_pointer);
    }
}

mir::input::InputEventTransformer::EventDispatcher::EventDispatcher(
    std::shared_ptr<MainLoop> const main_loop, input::InputSink* const sink, input::EventBuilder* const builder) :
    main_loop{main_loop},
    sink{sink},
    builder{builder}
{
}

void mir::input::InputEventTransformer::EventDispatcher::dispatch(mir::EventUPtr e)
{
    main_loop->spawn(
        [e = std::shared_ptr(std::move(e)), this]
        {
            sink->handle_input(e);
        });
}

void mir::input::InputEventTransformer::EventDispatcher::dispatch_key_event(
    std::optional<std::chrono::nanoseconds> timestamp, MirKeyboardAction action, xkb_keysym_t keysym, int scan_code)
{
    dispatch(builder->key_event(timestamp, action, keysym, scan_code));
}

void mir::input::InputEventTransformer::EventDispatcher::dispatch_pointer_event(
    std::optional<std::chrono::nanoseconds> timestamp,
    MirPointerAction action,
    MirPointerButtons buttons,
    std::optional<mir::geometry::PointF> position,
    mir::geometry::DisplacementF motion,
    MirPointerAxisSource axis_source,
    events::ScrollAxisH h_scroll,
    events::ScrollAxisV v_scroll)
{
    dispatch(builder->pointer_event(timestamp, action, buttons, position, motion, axis_source, h_scroll, v_scroll));
}
