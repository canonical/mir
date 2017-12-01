/*
 * Copyright © 2015 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "surface_input_dispatcher.h"

#include "mir/input/scene.h"
#include "mir/input/surface.h"
#include "mir/scene/observer.h"
#include "mir/scene/surface.h"
#include "mir/events/event_builders.h"
#include "mir_toolkit/mir_cookie.h"

#include <string.h>

#include <boost/throw_exception.hpp>
#include <stdexcept>
#include <algorithm>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace mev = mir::events;
namespace geom = mir::geometry;

namespace
{
struct InputDispatcherSceneObserver : public ms::Observer
{
    InputDispatcherSceneObserver(std::function<void(ms::Surface*)> const& on_removed)
        : on_removed(on_removed)
    {
    }
    void surface_added(ms::Surface* surface)
    {
        (void) surface;
    }
    void surface_removed(ms::Surface* surface)
    {
        on_removed(surface);
    }
    void surfaces_reordered()
    {
    }
    void scene_changed()
    {
    }

    void surface_exists(ms::Surface* surface)
    {
        (void) surface;
    }
    void end_observation()
    {
    }

    std::function<void(ms::Surface*)> const on_removed;
};

void deliver_without_relative_motion(
    std::shared_ptr<mi::Surface> const& surface,
    MirEvent const* ev,
    std::vector<uint8_t> const& drag_and_drop_handle)
{
    auto const* input_ev = mir_event_get_input_event(ev);
    auto const* pev = mir_input_event_get_pointer_event(input_ev);
    std::vector<uint8_t> cookie_data;
    if (mir_input_event_has_cookie(input_ev))
    {
        auto cookie = mir_input_event_get_cookie(input_ev);
        cookie_data.resize(mir_cookie_buffer_size(cookie));
        mir_cookie_to_buffer(cookie, cookie_data.data(), mir_cookie_buffer_size(cookie));
        mir_cookie_release(cookie);
    }
    auto const& bounds = surface->input_bounds();

    auto to_deliver = mev::make_event(mir_input_event_get_device_id(input_ev),
                                      std::chrono::nanoseconds{mir_input_event_get_event_time(input_ev)},
                                      cookie_data,
                                      mir_pointer_event_modifiers(pev),
                                      mir_pointer_event_action(pev),
                                      mir_pointer_event_buttons(pev),
                                      mir_pointer_event_axis_value(pev, mir_pointer_axis_x),
                                      mir_pointer_event_axis_value(pev, mir_pointer_axis_y),
                                      0.0f,
                                      0.0f,
                                      0.0f,
                                      0.0f);

    mev::transform_positions(*to_deliver, geom::Displacement{bounds.top_left.x.as_int(), bounds.top_left.y.as_int()});
    if (!drag_and_drop_handle.empty())
        mev::set_drag_and_drop_handle(*to_deliver, drag_and_drop_handle);
    surface->consume(to_deliver.get());
}

void deliver(
    std::shared_ptr<mi::Surface> const& surface,
    MirEvent const* ev,
    std::vector<uint8_t> const& drag_and_drop_handle)
{
    auto to_deliver = mev::clone_event(*ev);

    if (!drag_and_drop_handle.empty())
        mev::set_drag_and_drop_handle(*to_deliver, drag_and_drop_handle);

    auto const& bounds = surface->input_bounds();
    mev::transform_positions(*to_deliver, geom::Displacement{bounds.top_left.x.as_int(), bounds.top_left.y.as_int()});
    surface->consume(to_deliver.get());
}

}

mi::SurfaceInputDispatcher::SurfaceInputDispatcher(std::shared_ptr<mi::Scene> const& scene)
    : scene(scene),
      started(false)
{
    scene_observer = std::make_shared<InputDispatcherSceneObserver>([this](ms::Surface* s){surface_removed(s);});
    scene->add_observer(scene_observer);
}

mi::SurfaceInputDispatcher::~SurfaceInputDispatcher()
{
    scene->remove_observer(scene_observer);
}

namespace
{
bool compare_surfaces(std::shared_ptr<mi::Surface> const& input_surface, ms::Surface *surface)
{
    return input_surface.get() == static_cast<mi::Surface*>(surface);
}
}

void mi::SurfaceInputDispatcher::surface_removed(ms::Surface *surface)
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);

    auto strong_focus = focus_surface.lock();
    if (strong_focus && compare_surfaces(strong_focus, surface))
    {
        set_focus_locked(lg, nullptr);
    }

    for (auto& kv : pointer_state_by_id)
    {
        auto& state = kv.second;
        if (compare_surfaces(state.current_target, surface))
            state.current_target.reset();
        if (compare_surfaces(state.gesture_owner, surface))
            state.gesture_owner.reset();
    }

    for (auto& kv : touch_state_by_id)
    {
        auto& state = kv.second;
        if (compare_surfaces(state.gesture_owner, surface))
            state.gesture_owner.reset();
    }
}

void mi::SurfaceInputDispatcher::device_reset(MirInputDeviceId reset_device_id, std::chrono::nanoseconds /* when */)
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);

    if (!started)
        return;

    auto pointer_it = pointer_state_by_id.find(reset_device_id);
    if (pointer_it != pointer_state_by_id.end())
        pointer_state_by_id.erase(pointer_it);
    
    auto touch_it = touch_state_by_id.find(reset_device_id);
    if (touch_it != touch_state_by_id.end())
        touch_state_by_id.erase(touch_it);
}

bool mi::SurfaceInputDispatcher::dispatch_key(MirEvent const* kev)
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);

    if (!started)
        return false;

    auto strong_focus = focus_surface.lock();
    if (!strong_focus)
        return false;

    strong_focus->consume(kev);

    return true;
}

namespace
{
void for_pressed_buttons(MirPointerEvent const* pev, std::function<void(MirPointerButton)> const& exec)
{
    auto const buttons = {
        mir_pointer_button_primary,
        mir_pointer_button_secondary,
        mir_pointer_button_tertiary,
        mir_pointer_button_back,
        mir_pointer_button_forward
    };
    for (auto button : buttons)
        if (mir_pointer_event_button_state(pev, button)) exec(button);
}
    
bool is_gesture_terminator(MirPointerEvent const* pev)
{
    bool any_pressed = false;
    for_pressed_buttons(pev, [&any_pressed](MirPointerButton){ any_pressed = true; });
    return !any_pressed && mir_pointer_event_action(pev) == mir_pointer_action_button_up;
}
}

std::shared_ptr<mi::Surface> mi::SurfaceInputDispatcher::find_target_surface(geom::Point const& point)
{
    std::shared_ptr<mi::Surface> top_target = nullptr;
    scene->for_each([&top_target, &point](std::shared_ptr<mi::Surface> const& target) {
            if (target->input_area_contains(point))
                top_target = target;
    });
    return top_target;
}

void mi::SurfaceInputDispatcher::send_enter_exit_event(std::shared_ptr<mi::Surface> const& surface,
                                                       MirPointerEvent const* pev,
                                                       MirPointerAction action)
{
    auto surface_displacement = surface->input_bounds().top_left;
    auto const* input_ev = mir_pointer_event_input_event(pev);

    auto event = mev::make_event(mir_input_event_get_device_id(input_ev),
        std::chrono::nanoseconds(mir_input_event_get_event_time(input_ev)),
        std::vector<uint8_t>{},
        mir_pointer_event_modifiers(pev),
        action, mir_pointer_event_buttons(pev),
        mir_pointer_event_axis_value(pev, mir_pointer_axis_x) - surface_displacement.x.as_int(),
        mir_pointer_event_axis_value(pev, mir_pointer_axis_y) - surface_displacement.y.as_int(),
        mir_pointer_event_axis_value(pev, mir_pointer_axis_hscroll),
        mir_pointer_event_axis_value(pev, mir_pointer_axis_vscroll),
        mir_pointer_event_axis_value(pev, mir_pointer_axis_relative_x),
        mir_pointer_event_axis_value(pev, mir_pointer_axis_relative_y));

    if (!drag_and_drop_handle.empty())
        mev::set_drag_and_drop_handle(*event, drag_and_drop_handle);
    surface->consume(event.get());
}

mi::SurfaceInputDispatcher::PointerInputState& mi::SurfaceInputDispatcher::ensure_pointer_state(MirInputDeviceId id)
{
    pointer_state_by_id.insert(std::make_pair(id, PointerInputState()));
    return pointer_state_by_id[id];
}

mi::SurfaceInputDispatcher::TouchInputState& mi::SurfaceInputDispatcher::ensure_touch_state(MirInputDeviceId id)
{
    touch_state_by_id.insert(std::make_pair(id, TouchInputState()));
    return touch_state_by_id[id];
}

bool mi::SurfaceInputDispatcher::dispatch_pointer(MirInputDeviceId id, MirEvent const* ev)
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);
    auto const* input_ev = mir_event_get_input_event(ev);
    auto const* pev = mir_input_event_get_pointer_event(input_ev);
    auto action = mir_pointer_event_action(pev);
    auto& pointer_state = ensure_pointer_state(id);
    geom::Point event_x_y = { mir_pointer_event_axis_value(pev,mir_pointer_axis_x),
                              mir_pointer_event_axis_value(pev,mir_pointer_axis_y) };

    if (pointer_state.gesture_owner)
    {
        deliver(pointer_state.gesture_owner, ev, drag_and_drop_handle);

        auto const gesture_terminated = is_gesture_terminator(pev);

        if (gesture_terminated)
        {
            pointer_state.gesture_owner.reset();
        }

        if (gesture_terminated || !drag_and_drop_handle.empty())
        {
            auto target = find_target_surface(event_x_y);

            if (pointer_state.current_target != target)
            {
                if (pointer_state.current_target)
                    send_enter_exit_event(pointer_state.current_target, pev, mir_pointer_action_leave);

                pointer_state.current_target = target;
                if (target)
                    send_enter_exit_event(target, pev, mir_pointer_action_enter);

                if (!gesture_terminated)
                    pointer_state.gesture_owner = target;
            }
        }

        return true;
    }
    else if (action == mir_pointer_action_button_up)
    {
        // If we have an up but no gesture owner
        // then we never delivered the corresponding
        // down to anyone so we drop this event.
        return false;
    }
    else
    {
        auto target = find_target_surface(event_x_y);
        bool sent_ev = false;
        if (pointer_state.current_target != target)
        {
            if (pointer_state.current_target)
                send_enter_exit_event(pointer_state.current_target, pev, mir_pointer_action_leave);

            pointer_state.current_target = target;
            if (target)
                send_enter_exit_event(target, pev, mir_pointer_action_enter);

            sent_ev = true;
        }
        if (!target)
            return sent_ev;
        if (action == mir_pointer_action_button_down)
        {
            pointer_state.gesture_owner = target;
        }

        if (sent_ev)
        {
            if (action != mir_pointer_action_motion)
                deliver_without_relative_motion(target, ev, drag_and_drop_handle);
        }
        else
        {
            deliver(target, ev, drag_and_drop_handle);
        }
        return true;
    }
    return false;
}

namespace
{
bool is_gesture_start(MirTouchEvent const* tev)
{
    auto const point_count = mir_touch_event_point_count(tev);

    for (auto i = 0u; i != point_count; ++i)
        if (mir_touch_event_action(tev, i) != mir_touch_action_down)
            return false;

    return true;
}

bool is_gesture_end(MirTouchEvent const* tev)
{
    auto const point_count = mir_touch_event_point_count(tev);

    for (auto i = 0u; i != point_count; ++i)
        if (mir_touch_event_action(tev, i) != mir_touch_action_up)
            return false;

    return true;
}
}

bool mi::SurfaceInputDispatcher::dispatch_touch(MirInputDeviceId id, MirEvent const* ev)
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);
    auto const* input_ev = mir_event_get_input_event(ev);
    auto const* tev = mir_input_event_get_touch_event(input_ev);

    auto& gesture_owner = ensure_touch_state(id).gesture_owner;

    // We record the gesture_owner if the event signifies the start of a new
    // gesture. This prevents gesture ownership from transfering in the event
    // a gesture receiver closes mid gesture (e.g. when a surface closes mid
    // swipe we do not want the surface under to receive events). This also
    // allows a gesture to continue outside of the target surface, providing
    // it started in the target surface.
    if (is_gesture_start(tev))
    {
        geom::Point event_x_y = { mir_touch_event_axis_value(tev, 0, mir_touch_axis_x),
                                  mir_touch_event_axis_value(tev, 0, mir_touch_axis_y) };

        gesture_owner = find_target_surface(event_x_y);
    }

    if (gesture_owner)
    {
        deliver(gesture_owner, ev, drag_and_drop_handle);

        if (is_gesture_end(tev))
            gesture_owner.reset();

        return true;
    }
        
    return false;
}

bool mi::SurfaceInputDispatcher::dispatch(std::shared_ptr<MirEvent const> const& event)
{
    if (mir_event_get_type(event.get()) != mir_event_type_input)
        BOOST_THROW_EXCEPTION(std::logic_error("InputDispatcher got an unexpected event type"));
    
    auto iev = mir_event_get_input_event(event.get());
    auto id = mir_input_event_get_device_id(iev);
    switch (mir_input_event_get_type(iev))
    {
    case mir_input_event_type_key:
        return dispatch_key(event.get());
    case mir_input_event_type_touch:
        return dispatch_touch(id, event.get());
    case mir_input_event_type_pointer:
        return dispatch_pointer(id, event.get());
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("InputDispatcher got an input event of unknown type"));
    }
    
    return true;
}

void mi::SurfaceInputDispatcher::start()
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);

    started = true;
}

void mi::SurfaceInputDispatcher::stop()
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);

    pointer_state_by_id.clear();
    touch_state_by_id.clear();
    
    started = false;
}

void mi::SurfaceInputDispatcher::set_focus_locked(std::lock_guard<std::mutex> const&, std::shared_ptr<mi::Surface> const& target)
{
    focus_surface = target;
}

void mi::SurfaceInputDispatcher::set_focus(std::shared_ptr<mi::Surface> const& target)
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);
    set_focus_locked(lg, target);
}

void mi::SurfaceInputDispatcher::clear_focus()
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);
    set_focus_locked(lg, nullptr);
}

void mir::input::SurfaceInputDispatcher::set_drag_and_drop_handle(std::vector<uint8_t> const& handle)
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);
    drag_and_drop_handle = handle;
}

void mir::input::SurfaceInputDispatcher::clear_drag_and_drop_handle()
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);
    drag_and_drop_handle.clear();
}

