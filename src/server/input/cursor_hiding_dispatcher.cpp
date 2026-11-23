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

#include "cursor_hiding_dispatcher.h"

#include <mir/geometry/displacement.h>
#include <mir/input/device.h>
#include <mir/input/device_capability.h>
#include <mir/lockable_callback.h>
#include <mir/time/alarm.h>
#include <mir/time/alarm_factory.h>
#include <mir/time/clock.h>

#include <cmath>

namespace mi = mir::input;
namespace geom = mir::geometry;

namespace
{
float const default_reveal_threshold = 10.0f;
}

/// Takes the dispatcher's mutex before the alarm takes its own locks, so alarm methods
/// can safely be called while holding the dispatcher's mutex.
class mi::CursorHidingDispatcher::AlarmCallback : public LockableCallback
{
public:
    explicit AlarmCallback(CursorHidingDispatcher& self) : self{self} {}

    void operator()() override { self.alarm_fired_locked(); }
    void lock() override { self.mutex.lock(); }
    void unlock() override { self.mutex.unlock(); }

private:
    CursorHidingDispatcher& self;
};

mi::CursorHidingDispatcher::CursorHidingDispatcher(
    std::shared_ptr<InputDispatcher> const& next_dispatcher,
    std::function<void(bool hidden)> set_cursor_hidden,
    std::function<bool()> pointer_constrained,
    std::shared_ptr<time::Clock> const& clock,
    time::AlarmFactory& alarm_factory)
    : next_dispatcher{next_dispatcher},
      set_cursor_hidden{std::move(set_cursor_hidden)},
      pointer_constrained{std::move(pointer_constrained)},
      clock{clock},
      threshold{default_reveal_threshold},
      last_pointer_activity{clock->now()},
      alarm{alarm_factory.create_alarm(std::make_unique<AlarmCallback>(*this))}
{
}

mi::CursorHidingDispatcher::~CursorHidingDispatcher()
{
    alarm->cancel();
}

bool mi::CursorHidingDispatcher::dispatch(std::shared_ptr<MirEvent const> const& event)
{
    if (mir_event_get_type(event.get()) == mir_event_type_input)
    {
        auto const input_event = mir_event_get_input_event(event.get());
        switch (mir_input_event_get_type(input_event))
        {
        case mir_input_event_type_pointer:
            handle_pointer(mir_input_event_get_pointer_event(input_event));
            break;

        case mir_input_event_type_touch:
        {
            auto const touch_event = mir_input_event_get_touch_event(input_event);
            std::lock_guard lock{mutex};
            if (touch_enabled)
            {
                for (unsigned i = 0; i != mir_touch_event_point_count(touch_event); ++i)
                {
                    if (mir_touch_event_action(touch_event, i) == mir_touch_action_down)
                    {
                        hide_locked();
                        break;
                    }
                }
            }
            break;
        }

        case mir_input_event_type_key:
            break;

        default:
            break;
        }
    }

    auto const consumed = next_dispatcher->dispatch(event);

    if (mir_event_get_type(event.get()) == mir_event_type_input)
    {
        auto const input_event = mir_event_get_input_event(event.get());
        if (mir_input_event_get_type(input_event) == mir_input_event_type_key && !consumed)
        {
            auto const keyboard_event = mir_input_event_get_keyboard_event(input_event);
            std::lock_guard lock{mutex};
            if (keyboard_enabled && mir_keyboard_event_action(keyboard_event) == mir_keyboard_action_down)
            {
                hide_locked();
            }
        }
    }

    return consumed;
}

void mi::CursorHidingDispatcher::handle_pointer(MirPointerEvent const* event)
{
    geom::PointF const position{
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    auto const scrolled =
        mir_pointer_event_axis_value(event, mir_pointer_axis_vscroll) != 0 ||
        mir_pointer_event_axis_value(event, mir_pointer_axis_hscroll) != 0 ||
        mir_pointer_event_axis_value(event, mir_pointer_axis_vscroll_discrete) != 0 ||
        mir_pointer_event_axis_value(event, mir_pointer_axis_hscroll_discrete) != 0;

    std::lock_guard lock{mutex};

    buttons_held = mir_pointer_event_buttons(event) != 0;
    last_pointer_activity = clock->now();
    schedule_alarm_locked();

    if (hidden)
    {
        if (last_position)
        {
            auto const delta = position - *last_position;
            distance_moved += std::hypot(delta.dx.as_value(), delta.dy.as_value());
        }

        if (mir_pointer_event_action(event) == mir_pointer_action_button_down || scrolled || distance_moved >= threshold)
        {
            show_locked();
        }
    }

    last_position = position;
}

void mi::CursorHidingDispatcher::hide_locked()
{
    if (hidden || buttons_held || pointer_constrained())
    {
        return;
    }

    hidden = true;
    distance_moved = 0;
    alarm->cancel();
    set_cursor_hidden(true);
}

void mi::CursorHidingDispatcher::show_locked()
{
    hidden = false;
    set_cursor_hidden(false);
    schedule_alarm_locked();
}

void mi::CursorHidingDispatcher::schedule_alarm_locked()
{
    if (hidden || !inactivity_timeout)
    {
        alarm->cancel();
    }
    else
    {
        alarm->reschedule_for(last_pointer_activity + *inactivity_timeout);
    }
}

void mi::CursorHidingDispatcher::alarm_fired_locked()
{
    if (hidden || !inactivity_timeout)
    {
        return;
    }

    // Pointer events only record their time, rather than rescheduling the alarm every time
    if (clock->now() < last_pointer_activity + *inactivity_timeout)
    {
        schedule_alarm_locked();
        return;
    }

    hide_locked();

    if (!hidden)
    {
        // Hiding was blocked (e.g. by a held button), so wait for another full timeout
        last_pointer_activity = clock->now();
        schedule_alarm_locked();
    }
}

void mi::CursorHidingDispatcher::start()
{
    next_dispatcher->start();
}

void mi::CursorHidingDispatcher::stop()
{
    next_dispatcher->stop();
}

void mi::CursorHidingDispatcher::device_added(std::shared_ptr<Device> const& device)
{
    std::lock_guard lock{mutex};

    if (contains(device->capabilities(), DeviceCapability::pointer))
    {
        // The cursor controller shows the cursor when the first pointing device is attached
        if (pointing_devices.empty())
        {
            hidden = false;
            last_pointer_activity = clock->now();
            schedule_alarm_locked();
        }
        pointing_devices.insert(device->id());
    }
}

void mi::CursorHidingDispatcher::device_changed(std::shared_ptr<Device> const&)
{
}

void mi::CursorHidingDispatcher::device_removed(std::shared_ptr<Device> const& device)
{
    std::lock_guard lock{mutex};
    pointing_devices.erase(device->id());
    if (pointing_devices.empty())
    {
        alarm->cancel();
    }
}

void mi::CursorHidingDispatcher::changes_complete()
{
}

void mi::CursorHidingDispatcher::hide_on_touch(bool enabled)
{
    std::lock_guard lock{mutex};
    touch_enabled = enabled;
}

void mi::CursorHidingDispatcher::hide_on_keyboard(bool enabled)
{
    std::lock_guard lock{mutex};
    keyboard_enabled = enabled;
}

void mi::CursorHidingDispatcher::hide_after_inactivity(std::optional<std::chrono::milliseconds> timeout)
{
    std::lock_guard lock{mutex};
    inactivity_timeout = timeout;
    last_pointer_activity = clock->now();
    schedule_alarm_locked();
}

void mi::CursorHidingDispatcher::reveal_threshold(float distance)
{
    std::lock_guard lock{mutex};
    threshold = distance;
}
