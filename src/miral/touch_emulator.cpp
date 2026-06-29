/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/input/touch_visualizer.h"
#include <miral/touch_emulator.h>
#include <miral/prepend_event_filter.h>

#include <mir/events/touch_contact.h>
#include <mir/geometry/point.h>
#include <mir/input/device_capability.h>
#include <mir/input/event_builder.h>
#include <mir/input/input_device_registry.h>
#include <mir/input/input_sink.h>
#include <mir/input/virtual_input_device.h>
#include <mir/main_loop.h>
#include <mir/server.h>

#include <mir_toolkit/events/input/pointer_event.h>
#include <mir_toolkit/events/input/input_event.h>
#include <mir_toolkit/events/event.h>
#include <mir_toolkit/events/enums.h>

#include <mutex>

namespace mi   = mir::input;
namespace mev  = mir::events;
namespace geom = mir::geometry;

class miral::TouchEmulator::Self
{
public:
    Self() : filter{[this](MirEvent const* event) { return handle(event); }} {}

    void init(mir::Server& server)
    {
        filter(server);

        touch_device = std::make_shared<mi::VirtualInputDevice>(
            "touch-emulator", mi::DeviceCapability::touchscreen);

        server.add_init_callback([this, &server]
        {
            server.the_input_device_registry()->add_device(touch_device);
            main_loop = server.the_main_loop();
        });

        server.add_stop_callback([this, &server]
        {
            server.the_input_device_registry()->remove_device(touch_device);
            main_loop.reset();
        });
    }

    bool handle(MirEvent const* event)
    {
        if (mir_event_get_type(event) != mir_event_type_input)
            return false;

        auto const* input_ev = mir_event_get_input_event(event);
        if (mir_input_event_get_type(input_ev) != mir_input_event_type_pointer)
            return false;

        auto const* pev = mir_input_event_get_pointer_event(input_ev);

        // Ctrl+drag passes through unchanged for normal pointer interaction.
        if (mir_pointer_event_modifiers(pev) & mir_input_event_modifier_ctrl)
            return false;

        auto const action  = mir_pointer_event_action(pev);
        auto const buttons = mir_pointer_event_buttons(pev);
        float const x = mir_pointer_event_axis_value(pev, mir_pointer_axis_x);
        float const y = mir_pointer_event_axis_value(pev, mir_pointer_axis_y);
        auto const ts = std::chrono::nanoseconds{mir_input_event_get_event_time(input_ev)};

        std::lock_guard lock{mutex};

        if (action == mir_pointer_action_button_down &&
            (buttons & mir_pointer_button_primary))
        {
            dragging = true;
            dispatch_touch(mir_touch_action_down, x, y, ts);
            return true;
        }
        else if (action == mir_pointer_action_motion && dragging)
        {
            dispatch_touch(mir_touch_action_change, x, y, ts);
            return true;
        }
        else if (action == mir_pointer_action_button_up && dragging &&
                 !(buttons & mir_pointer_button_primary))
        {
            dragging = false;
            dispatch_touch(mir_touch_action_up, x, y, ts);
            return true;
        }

        // Consume all other pointer events so they don't reach surfaces.
        return true;
    }

private:
    void dispatch_touch(
        MirTouchAction action,
        float x,
        float y,
        std::chrono::nanoseconds timestamp)
    {
        auto const ml = main_loop.lock();
        if (!ml)
            return;

        // Enqueue the dispatch so it runs after the current filter invocation
        // returns.  Calling sink->handle_input() from inside the filter would
        // re-enter the composite event filter while it is already executing,
        // causing a deadlock.
        ml->enqueue(
            this,
            [this, action, x, y, timestamp]
            {
                static MirTouchId constexpr touch_id = 0;

                mev::TouchContact const contact{
                    touch_id,
                    action,
                    mir_touch_tooltype_finger,
                    geom::PointF{x, y},
                    1.0f,  // pressure
                    8.0f,  // touch_major
                    8.0f,  // touch_minor
                    0.0f}; // orientation

                touch_device->if_started_then(
                    [&contact, timestamp](mi::InputSink* sink, mi::EventBuilder* builder)
                    {
                        auto ev = builder->touch_event(timestamp, {contact});
                        sink->handle_input(std::move(ev));
                    });
            });
    }

    std::mutex mutex;
    bool dragging{false};
    std::shared_ptr<mi::VirtualInputDevice> touch_device;
    std::weak_ptr<mir::MainLoop> main_loop;
    miral::PrependEventFilter filter;
};

miral::TouchEmulator::TouchEmulator()
    : self(std::make_shared<Self>())
{
}

void miral::TouchEmulator::operator()(mir::Server& server)
{
    self->init(server);
}
