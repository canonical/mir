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
#include "mir/scene/null_observer.h"
#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"
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
struct InputDispatcherSceneObserver :
    public ms::NullObserver,
    public ms::NullSurfaceObserver,
    public std::enable_shared_from_this<InputDispatcherSceneObserver>
{
    InputDispatcherSceneObserver(
        std::function<void(std::shared_ptr<ms::Surface>)> const& on_removed,
        std::function<void(ms::Surface const*)> const& on_surface_moved,
        std::function<void()> const& on_surface_resized)
        : on_removed(on_removed),
          on_surface_moved{on_surface_moved},
          on_surface_resized{on_surface_resized}
    {
    }

    void surface_added(std::shared_ptr<ms::Surface> surface) override
    {
        surface->add_observer(shared_from_this());
    }

    void surface_removed(std::shared_ptr<ms::Surface> surface) override
    {
        on_removed(surface);
    }

    void surface_exists(std::shared_ptr<ms::Surface> surface) override
    {
        surface->add_observer(shared_from_this());
    }

    void attrib_changed(ms::Surface const*, MirWindowAttrib /*attrib*/, int /*value*/) override
    {
        // TODO: Do we need to listen to visibility events?
    }

    void resized_to(ms::Surface const*, mir::geometry::Size const& /*size*/) override
    {
        on_surface_resized();
    }

    void moved_to(ms::Surface const* surf, mir::geometry::Point const& /*top_left*/) override
    {
        on_surface_moved(surf);
    }

    void hidden_set_to(ms::Surface const*, bool /*hide*/) override
    {
        // TODO: Do we need to listen to this?
    }

    std::function<void(std::shared_ptr<ms::Surface>)> const on_removed;
    std::function<void(ms::Surface const*)> const on_surface_moved;
    std::function<void()> const on_surface_resized;
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
    scene_observer = std::make_shared<InputDispatcherSceneObserver>(
        std::bind(
            std::mem_fn(&SurfaceInputDispatcher::surface_removed),
            this,
            std::placeholders::_1),
        std::bind(
            std::mem_fn(&SurfaceInputDispatcher::surface_moved),
            this,
            std::placeholders::_1),
        std::bind(
            std::mem_fn(&SurfaceInputDispatcher::surface_resized),
            this));
    scene->add_observer(scene_observer);
}

mi::SurfaceInputDispatcher::~SurfaceInputDispatcher()
{
    scene->remove_observer(scene_observer);
    scene->for_each(
        [this](auto surface)
        {
            // Everything *should* be a scene::Surface, but let's not crash if it isn't.
            if (auto scene_surf = std::dynamic_pointer_cast<ms::Surface>(surface))
            {
                // We *know* scene_observer is an InputDispatcherSceneObserver
                scene_surf->remove_observer(
                    std::static_pointer_cast<InputDispatcherSceneObserver>(scene_observer));
            }
        });
}

namespace
{
bool compare_surfaces(std::shared_ptr<mi::Surface> const& input_surface, ms::Surface *surface)
{
    return input_surface.get() == static_cast<mi::Surface*>(surface);
}
}

void mi::SurfaceInputDispatcher::surface_removed(std::shared_ptr<ms::Surface> surface)
{
    std::lock_guard<std::mutex> lg(dispatcher_mutex);

    auto strong_focus = focus_surface.lock();
    if (strong_focus && compare_surfaces(strong_focus, surface.get()))
    {
        set_focus_locked(lg, nullptr);
    }

    for (auto& kv : pointer_state_by_id)
    {
        auto& state = kv.second;
        if (compare_surfaces(state.current_target, surface.get()))
            state.current_target.reset();
        if (compare_surfaces(state.gesture_owner, surface.get()))
            state.gesture_owner.reset();
    }

    for (auto& kv : touch_state_by_id)
    {
        auto& state = kv.second;
        if (compare_surfaces(state.gesture_owner, surface.get()))
            state.gesture_owner.reset();
    }
}

namespace
{
struct SceneChangeContext
{
    MirInputEvent const* const iev;
    MirPointerEvent const* const pev;
    std::shared_ptr<mi::Surface>& current_target;
    std::shared_ptr<mi::Surface> const target_surface;
};

SceneChangeContext context_for_event(
    MirEvent const* last_pointer_event,
    std::function<std::shared_ptr<mi::Surface>*(MirInputDeviceId)> const& get_current_target,
    std::function<std::shared_ptr<mi::Surface>(geom::Point const&)> const& surface_under_point)
{
    auto const iev = mir_event_get_input_event(last_pointer_event);
    auto const pev = mir_input_event_get_pointer_event(iev);
    geom::Point const event_x_y = {
        mir_pointer_event_axis_value(pev,mir_pointer_axis_x),
        mir_pointer_event_axis_value(pev,mir_pointer_axis_y)
    };

    return SceneChangeContext{
        iev,
        pev,
        *get_current_target(mir_input_event_get_device_id(iev)),
        surface_under_point(event_x_y)
    };
}

using SendEnterExitFn = std::function<void(std::shared_ptr<mi::Surface> const&, MirPointerEvent const*, MirPointerAction action)>;

bool dispatch_scene_change_enter_exit_events(
    SceneChangeContext const& ctx,
    SendEnterExitFn const& send_enter_exit_event)
{
    /*
     * Check if the top-most surface that's under the pointer is the same as the last
     * time we sent and input event…
     */
    auto const target_changed = ctx.current_target != ctx.target_surface;
    if (target_changed)
    {
        if (ctx.current_target)
        {
            /*
             * We have been sending pointer events to a surface, but it's no longer
             * top-most under the cursor; we need to send a leave event.
             */
            auto const event = mev::make_event(
                mir_input_event_get_device_id(ctx.iev),
                std::chrono::nanoseconds{std::chrono::steady_clock::now().time_since_epoch()},
                std::vector<uint8_t>{},
                0, // TODO: We need the current keyboard state
                mir_pointer_action_leave,
                mir_pointer_event_buttons(ctx.pev),
                mir_pointer_event_axis_value(ctx.pev, mir_pointer_axis_x),
                mir_pointer_event_axis_value(ctx.pev, mir_pointer_axis_y),
                0, 0, // No scrolling
                0, 0  // No relative motion
            );

            auto const synth_iev = mir_event_get_input_event(event.get());
            auto const synth_pev = mir_input_event_get_pointer_event(synth_iev);
            send_enter_exit_event(ctx.current_target, synth_pev, mir_pointer_action_leave);
        }
        if (ctx.target_surface)
        {
            /*
             * A new surface is top-most underneath the cursor; send a pointer enter event
             */
            auto const event = mev::make_event(
                mir_input_event_get_device_id(ctx.iev),
                std::chrono::nanoseconds{std::chrono::steady_clock::now().time_since_epoch()},
                std::vector<uint8_t>{},
                0, // TODO: We need the current keyboard state
                mir_pointer_action_enter,
                mir_pointer_event_buttons(ctx.pev),
                mir_pointer_event_axis_value(ctx.pev, mir_pointer_axis_x),
                mir_pointer_event_axis_value(ctx.pev, mir_pointer_axis_y),
                0, 0, // No scrolling
                0, 0  // No relative motion
            );

            auto const synth_iev = mir_event_get_input_event(event.get());
            auto const synth_pev = mir_input_event_get_pointer_event(synth_iev);
            send_enter_exit_event(ctx.target_surface, synth_pev, mir_pointer_action_enter);
        }
    }
    return target_changed;
}

void send_motion_event_to_moved_surface(
    SceneChangeContext const& ctx,
    ms::Surface const* moved_surface,
    std::function<void(std::shared_ptr<mi::Surface> const&, MirEvent const*)> const& send_motion)
{
    if ((ctx.target_surface.get() == moved_surface) &&
        ctx.current_target == ctx.target_surface)
    {
        /*
         * The surface that has moved was the top-most surface before;
         * it has already received a pointer_enter event, so we need to send
         * a motion event for the movement
         */
        auto const event = mev::make_event(
            mir_input_event_get_device_id(ctx.iev),
            std::chrono::nanoseconds{std::chrono::steady_clock::now().time_since_epoch()},
            std::vector<uint8_t>{},
            0, // TODO: We need the current keyboard state
            mir_pointer_action_motion,
            0,
            mir_pointer_event_axis_value(ctx.pev, mir_pointer_axis_x),
            mir_pointer_event_axis_value(ctx.pev, mir_pointer_axis_y),
            0, 0, // No scrolling
            0, 0  // No relative motion
        );

        send_motion(ctx.target_surface, event.get());
    }
}
}

void mi::SurfaceInputDispatcher::surface_moved(ms::Surface const* moved_surface)
{
    std::lock_guard<std::mutex> lock{dispatcher_mutex};

    if (!last_pointer_event)
        return;

    auto ctx = context_for_event(
        last_pointer_event.get(),
        [this](auto id) { return &this->ensure_pointer_state(id).current_target; },
        [this](auto point) { return this->find_target_surface(point); });

    // If we're in a move/resize gesture we don't need to synthesize an event
    if (ensure_pointer_state(mir_input_event_get_device_id(ctx.iev)).gesture_owner)
        return;

    auto const entered_surface_changed = dispatch_scene_change_enter_exit_events(
        ctx,
        [this](auto surf, auto pev, auto action) { this->send_enter_exit_event(surf, pev, action); });
    if (entered_surface_changed)
    {
        ctx.current_target = ctx.target_surface;
    } else
    {
        send_motion_event_to_moved_surface(
            ctx,
            moved_surface,
            [this](auto surf, auto ev) { deliver_without_relative_motion(surf, ev, drag_and_drop_handle); });
    }
}

void mi::SurfaceInputDispatcher::surface_resized()
{
    std::lock_guard<std::mutex> lock{dispatcher_mutex};

    if (!last_pointer_event)
        return;

    auto ctx = context_for_event(
        last_pointer_event.get(),
        [this](auto id) { return &this->ensure_pointer_state(id).current_target; },
        [this](auto point) { return this->find_target_surface(point); });

    auto const entered_surface_changed = dispatch_scene_change_enter_exit_events(
        ctx,
        [this](auto surf, auto pev, auto action) { this->send_enter_exit_event(surf, pev, action); });

    if (entered_surface_changed)
    {
        ctx.current_target = ctx.target_surface;
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

bool mi::SurfaceInputDispatcher::dispatch_pointer(MirInputDeviceId id, std::shared_ptr<MirEvent const> const& event)
{
    auto const ev = event.get();
    std::lock_guard<std::mutex> lg(dispatcher_mutex);
    last_pointer_event = event;
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
        return dispatch_pointer(id, event);
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
    last_pointer_event.reset();
    
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

