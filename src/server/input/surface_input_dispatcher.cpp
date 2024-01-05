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

#include "surface_input_dispatcher.h"

#include "mir/input/scene.h"
#include "mir/input/surface.h"
#include "mir/scene/null_observer.h"
#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/events/event_helpers.h"
#include "mir/events/pointer_event.h"

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
        std::function<void()> const& on_surface_resized,
        std::function<void()> const& on_scene_changed)
        : on_removed(on_removed),
          on_surface_moved{on_surface_moved},
          on_surface_resized{on_surface_resized},
          on_scene_changed{on_scene_changed}
    {
    }

    void surface_added(std::shared_ptr<ms::Surface> const& surface) override
    {
        surface->register_interest(shared_from_this(), mir::immediate_executor);
    }

    void surface_removed(std::shared_ptr<ms::Surface> const& surface) override
    {
        on_removed(surface);
    }

    void scene_changed() override
    {
        on_scene_changed();
    }

    void surface_exists(std::shared_ptr<ms::Surface> const& surface) override
    {
        surface->register_interest(shared_from_this());
    }

    void attrib_changed(ms::Surface const*, MirWindowAttrib /*attrib*/, int /*value*/) override
    {
        // TODO: Do we need to listen to visibility events?
    }

    void content_resized_to(ms::Surface const*, mir::geometry::Size const& /*size*/) override
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
    std::function<void()> const on_scene_changed;
};

void set_local_positions_based_on_surface_input_bounds(
    MirEvent& event,
    mir::geometry::Rectangle const& input_bounds)
{
    mev::map_positions(event, [&](auto global, auto local)
        {
            local = global - geom::DisplacementF{as_displacement(input_bounds.top_left)};
            return std::make_pair(global, local);
        });
}

void deliver_without_relative_motion(std::shared_ptr<mi::Surface> const& surface, MirEvent const* ev)
{
    auto const* input_ev = mir_event_get_input_event(ev);
    auto const* pev = mir_input_event_get_pointer_event(input_ev);
    std::vector<uint8_t> cookie_data;
    auto const& bounds = surface->input_bounds();
    auto to_deliver = mev::make_pointer_event(
        mir_input_event_get_device_id(input_ev),
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

    set_local_positions_based_on_surface_input_bounds(*to_deliver, bounds);
    surface->consume(std::move(to_deliver));
}

void deliver(std::shared_ptr<mi::Surface> const& surface, MirEvent const* ev)
{
    auto to_deliver = mev::clone_event(*ev);

    auto const& bounds = surface->input_bounds();
    set_local_positions_based_on_surface_input_bounds(*to_deliver, bounds);
    surface->consume(std::move(to_deliver));
}

}

mi::SurfaceInputDispatcher::SurfaceInputDispatcher(std::shared_ptr<mi::Scene> const& scene)
    : scene(scene),
      screen_is_locked(false)
{
    scene_observer = std::make_shared<InputDispatcherSceneObserver>(
        [this](std::shared_ptr<ms::Surface> const& s) { surface_removed(s); },
        [this](scene::Surface const* s) { surface_moved(s); },
        [this] { surface_resized(); },
        [this] { scene_changed(); });
    scene->add_observer(scene_observer);
}

mi::SurfaceInputDispatcher::~SurfaceInputDispatcher()
{
    scene->remove_observer(scene_observer);

    // It's safe to destroy a surface observer without unregistering it
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
    std::lock_guard lg(dispatcher_mutex);

    auto strong_focus = focus_surface.lock();
    if (strong_focus && compare_surfaces(strong_focus, surface.get()))
    {
        set_focus_locked(lg, nullptr);
    }

    if (compare_surfaces(current_target, surface.get()))
        current_target.reset();
    if (compare_surfaces(gesture_owner, surface.get()))
        gesture_owner.reset();

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
    std::shared_ptr<mi::Surface>& current_target,
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
        current_target,
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
            auto const event = mev::make_pointer_event(
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
            auto const event = mev::make_pointer_event(
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
        auto const event = mev::make_pointer_event(
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
    std::lock_guard lock{dispatcher_mutex};

    if (!last_pointer_event)
        return;

    auto ctx = context_for_event(
        last_pointer_event.get(),
        current_target,
        [this](auto point) { return scene->input_surface_at(point); });

    // If we're in a move/resize gesture we don't need to synthesize an event
    if (gesture_owner)
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
            [](auto surf, auto ev) { deliver_without_relative_motion(surf, ev); });
    }
}

void mi::SurfaceInputDispatcher::surface_resized()
{
    std::lock_guard lock{dispatcher_mutex};

    if (!last_pointer_event)
        return;

    auto ctx = context_for_event(
        last_pointer_event.get(),
        current_target,
        [this](auto point) { return scene->input_surface_at(point); });

    auto const entered_surface_changed = dispatch_scene_change_enter_exit_events(
        ctx,
        [this](auto surf, auto pev, auto action) { this->send_enter_exit_event(surf, pev, action); });

    if (entered_surface_changed)
    {
        ctx.current_target = ctx.target_surface;
    }
}

void mi::SurfaceInputDispatcher::scene_changed()
{
    std::lock_guard lock{dispatcher_mutex};
    bool const screen_is_locked_new = scene->screen_is_locked();
    if (screen_is_locked != screen_is_locked_new && screen_is_locked_new)
    {
        auto const focused = focus_surface.lock();
        if (focused && !focused->visible_on_lock_screen())
        {
            set_focus_locked(lock, nullptr);
        }
    }
    screen_is_locked = screen_is_locked_new;
}

bool mi::SurfaceInputDispatcher::dispatch_key(std::shared_ptr<MirEvent const> const& ev)
{
    keyboard_multiplexer.keyboard_event(ev);
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

void mi::SurfaceInputDispatcher::send_enter_exit_event(std::shared_ptr<mi::Surface> const& surface,
                                                       MirPointerEvent const* pev,
                                                       MirPointerAction action)
{
    auto const* input_ev = mir_pointer_event_input_event(pev);
    auto event = mev::clone_event(*mir_input_event_get_event(input_ev));
    auto const pointer_ev = static_cast<MirPointerEvent*>(event.get());
    pointer_ev->set_action(action);
    if (pointer_ev->position())
    {
        set_local_positions_based_on_surface_input_bounds(*event, surface->input_bounds());
    }

    surface->consume(std::move(event));
}

mi::SurfaceInputDispatcher::TouchInputState& mi::SurfaceInputDispatcher::ensure_touch_state(MirInputDeviceId id)
{
    touch_state_by_id.insert(std::make_pair(id, TouchInputState()));
    return touch_state_by_id[id];
}

bool mi::SurfaceInputDispatcher::dispatch_pointer(MirInputDeviceId /*id*/, std::shared_ptr<MirEvent const> const& event)
{
    auto const ev = event.get();
    std::unique_lock lg(dispatcher_mutex);
    last_pointer_event = event;
    auto const* input_ev = mir_event_get_input_event(ev);
    auto const* pev = mir_input_event_get_pointer_event(input_ev);
    auto action = mir_pointer_event_action(pev);
    geom::Point event_x_y = { mir_pointer_event_axis_value(pev,mir_pointer_axis_x),
                              mir_pointer_event_axis_value(pev,mir_pointer_axis_y) };

    if (gesture_owner && dispatch_to_gesture_owner)
    {
        deliver(gesture_owner, ev);

        if (is_gesture_terminator(pev))
        {
            gesture_owner.reset();

            auto target = scene->input_surface_at(event_x_y);

            if (current_target != target)
            {
                if (current_target)
                    send_enter_exit_event(current_target, pev, mir_pointer_action_leave);

                current_target = target;
                if (target)
                    send_enter_exit_event(target, pev, mir_pointer_action_enter);
            }
        }

        return true;
    }
    else
    {
        auto const target = scene->input_surface_at(event_x_y);
        bool sent_ev = false;
        if (current_target != target)
        {
            if (current_target)
                send_enter_exit_event(current_target, pev, mir_pointer_action_leave);

            current_target = target;
            if (target)
                send_enter_exit_event(target, pev, mir_pointer_action_enter);

            sent_ev = true;
        }

        if (target)
        {
            if (action == mir_pointer_action_button_down)
            {
                gesture_owner = target;
            }

            if (sent_ev)
            {
                if (action != mir_pointer_action_motion)
                    deliver_without_relative_motion(target, ev);
            }
            else
            {
                deliver(target, ev);
            }
            sent_ev = true;
        }

        if (is_gesture_terminator(pev))
        {
            gesture_owner.reset();
            dispatch_to_gesture_owner = true;
            auto end_gesture = on_end_gesture;
            on_end_gesture = []{};
            lg.unlock();

            end_gesture();
        }

        return sent_ev;
    }
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
    std::lock_guard lg(dispatcher_mutex);
    auto const* input_ev = mir_event_get_input_event(ev);
    auto const* tev = mir_input_event_get_touch_event(input_ev);

    auto& gesture_owner = ensure_touch_state(id).gesture_owner;

    // We record the gesture_owner if the event signifies the start of a new
    // gesture. This prevents gesture ownership from transfering in the event
    // a gesture receiver closes mid-gesture (e.g. when a surface closes mid
    // swipe we do not want the surface under to receive events). This also
    // allows a gesture to continue outside the target surface, providing
    // it started in the target surface.
    if (is_gesture_start(tev))
    {
        geom::Point event_x_y = { mir_touch_event_axis_value(tev, 0, mir_touch_axis_x),
                                  mir_touch_event_axis_value(tev, 0, mir_touch_axis_y) };

        gesture_owner = scene->input_surface_at(event_x_y);
    }

    if (gesture_owner)
    {
        deliver(gesture_owner, ev);

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
    case mir_input_event_type_keyboard_resync:
        return dispatch_key(event);
    case mir_input_event_type_touch:
        return dispatch_touch(id, event.get());
    case mir_input_event_type_pointer:
        return dispatch_pointer(id, event);
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("InputDispatcher got an input event of unknown type"));
    }
}

void mi::SurfaceInputDispatcher::start()
{
}

void mi::SurfaceInputDispatcher::stop()
{
    std::lock_guard lg(dispatcher_mutex);

    gesture_owner.reset();
    current_target.reset();
    touch_state_by_id.clear();
    last_pointer_event.reset();
}

void mi::SurfaceInputDispatcher::set_focus_locked(std::lock_guard<std::mutex> const&, std::shared_ptr<mi::Surface> const& target)
{
    if (screen_is_locked && target && !target->visible_on_lock_screen())
    {
        return;
    }
    focus_surface = target;
    keyboard_multiplexer.keyboard_focus_set(target);
}

void mi::SurfaceInputDispatcher::set_focus(std::shared_ptr<mi::Surface> const& target)
{
    std::lock_guard lg(dispatcher_mutex);
    set_focus_locked(lg, target);
}

void mi::SurfaceInputDispatcher::clear_focus()
{
    std::lock_guard lg(dispatcher_mutex);
    set_focus_locked(lg, nullptr);
}

void mir::input::SurfaceInputDispatcher::register_interest(std::weak_ptr<KeyboardObserver> const& observer)
{
    keyboard_multiplexer.register_interest(observer);
}

void mir::input::SurfaceInputDispatcher::register_interest(
    std::weak_ptr<KeyboardObserver> const& observer,
    Executor& executor)
{
    keyboard_multiplexer.register_interest(observer, executor);
}

void mir::input::SurfaceInputDispatcher::unregister_interest(KeyboardObserver const& observer)
{
    keyboard_multiplexer.unregister_interest(observer);
}

void mir::input::SurfaceInputDispatcher::disable_dispatch_to_gesture_owner(std::function<void()> on_end_gesture)
{
    std::lock_guard lg(dispatcher_mutex);
    dispatch_to_gesture_owner = false;
    this->on_end_gesture = std::move(on_end_gesture);
}
