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

#include <miral/magnifier.h>

#include "magnifier_handle_indicator.h"
#include "magnifier_layout.h"
#include "render_scene_into_surface.h"

#include <miral/live_config.h>
#include <mir/events/event.h>
#include <mir/events/input_event.h>
#include <mir/events/pointer_event.h>
#include <mir/events/touch_event.h>
#include <mir/log.h>
#include <mir/server.h>
#include <mir/synchronised.h>
#include <mir/graphics/display.h>
#include <mir/graphics/display_configuration.h>
#include <mir/graphics/display_configuration_observer.h>
#include <mir/geometry/rectangles.h>
#include <mir/input/composite_event_filter.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/input/event_filter.h>
#include <mir/scene/surface.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/scene/basic_surface.h>
#include <mir/shell/surface_stack.h>
#include <mir/observer_registrar.h>
#include <mir/main_loop.h>

#include <glm/gtc/matrix_transform.hpp>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace mc = mir::compositor;

namespace
{
auto const default_capture_width = 150;
auto const default_capture_height = 150;
auto const min_magnification = 1.0f;
auto const default_magnification = 1.25f;
auto const max_magnification = 8.0f;

/// Magnification step applied by each zoom button press.
auto const zoom_step = 0.25f;

class Handle
{
public:
    Handle() = default;

    void init(mir::Server& server, miral::HandleKind kind, mc::CompositorID capture_compositor_id)
    {
        indicator = std::make_shared<miral::HandleIndicator>(
            handle_rect,
            kind,
            capture_compositor_id,
            server.the_buffer_allocator(),
            server.the_scene_report(),
            server.the_display_configuration_observer_registrar());
        handle_surface_stack = server.the_surface_stack();

        server.the_surface_stack()->add_surface(indicator, mi::InputReceptionMode::normal);
        indicator->set_cursor_image(server.the_default_cursor_image());
    }

    void reset()
    {
        detach_observer();
        if (auto const surface_stack = handle_surface_stack.lock())
            surface_stack->remove_surface(indicator);
        indicator.reset();
    }

    template<typename ObserverType, typename... Args>
    void attach_observer(Args&&... args)
    {
        if (!indicator)
            return;

        observer = std::make_shared<ObserverType>(std::forward<Args>(args)...);
        indicator->register_interest(observer);
    }

    void detach_observer()
    {
        if (!observer)
            return;

        indicator->unregister_interest(*observer);
        observer.reset();
    }

    void move_to(geom::Point const& pos)
    {
        if (indicator)
            indicator->move_to(pos);
    }

    void show()
    {
        if (indicator)
            indicator->show();
    }

    void hide()
    {
        if (indicator)
            indicator->hide();
    }

private:
    static constexpr geom::Rectangle handle_rect{
        {0, 0},
        {
            geom::Width{miral::MagnifierLayout::handle_diameter},
            geom::Height{miral::MagnifierLayout::handle_diameter}}};

    std::shared_ptr<miral::HandleIndicator> indicator;
    std::weak_ptr<msh::SurfaceStack> handle_surface_stack;
    std::shared_ptr<ms::NullSurfaceObserver> observer;
};

struct DisplayConfigObserver;
struct CursorObserver;
struct State
{
    auto layout() const -> miral::MagnifierLayout
    {
        return {screen_bounds, visual_size, magnification};
    }

    auto placement_for(ms::Surface const& surf) const -> miral::MagnifierLayout::Placement
    {
        return {
            .capture_area = render_scene_into_surface.capture_area(),
            .surface_top_left = surf.top_left(),
            .visual_size = visual_size};
    }

    auto point_is_on_magnifier_surface(geom::PointF const& point) const -> bool
    {
        if (follow_cursor || !default_enabled)
            return false;

        auto const surf = surface.lock();
        if (!surf)
            return false;

        return layout().contains_content(point, placement_for(*surf));
    }

    void apply_geometry(miral::MagnifierLayout::Placement const& placement)
    {
        visual_size = placement.visual_size;
        render_scene_into_surface.capture_area(placement.capture_area);

        if (!follow_cursor)
        {
            auto const positions = layout().handle_positions(placement);
            drag.move_to(positions.drag);
            resize.move_to(positions.resize);
            zoom_in.move_to(positions.zoom_in);
            zoom_out.move_to(positions.zoom_out);
        }

        if (auto const surf = surface.lock())
        {
            surf->move_to(placement.surface_top_left);
            surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
        }
    }

    void show_all_handles()
    {
        for (auto* h : {&drag, &resize, &zoom_in, &zoom_out})
            h->show();
    }

    void hide_all_handles()
    {
        for (auto* h : {&drag, &resize, &zoom_in, &zoom_out})
            h->hide();
    }

    /// Unregisters and discards the observer on each handle that has one.
    void detach_observers()
    {
        for (auto* h : {&drag, &resize, &zoom_in, &zoom_out})
            h->detach_observer();
    }

    std::weak_ptr<ms::Surface> surface;
    Handle drag;
    Handle resize;
    Handle zoom_in;
    Handle zoom_out;
    geom::Point cursor_pos;
    geom::Rectangles screen_bounds;
    float magnification{default_magnification};
    /// The physical (screen) size of the magnifier.  When the magnification
    /// level changes the logical capture size is recalculated from this so
    /// that the magnifier's on-screen footprint stays the same.
    geom::Size visual_size;
    bool default_enabled{false};
    bool follow_cursor{true};
    miral::RenderSceneIntoSurface render_scene_into_surface;
};
}

class miral::Magnifier::Self
{
public:
    Self()
    {
        state.lock()
            ->render_scene_into_surface
            .capture_area(geom::Rectangle{{300, 300}, geom::Size(default_capture_width, default_capture_height)})
            .overlay_cursor(false);
    }

    void init(mir::Server& server)
    {
        auto const s = state.lock();
        s->render_scene_into_surface.on_surface_ready(
            [this](auto const& surf)
            {
                surf->set_depth_layer(mir_depth_layer_always_on_top);
                surf->set_focus_mode(mir_focus_mode_disabled);
                auto s = state.lock();

                if (s->default_enabled)
                    surf->show();
                else
                    surf->hide();

                s->surface = surf;
            });

        s->render_scene_into_surface(server);

        server.add_init_callback(
            [&]
            {
                input_filter = std::make_shared<InputFilter>(state);
                server.the_composite_event_filter()->prepend(input_filter);

                server.the_main_loop()->spawn([=, this, &server] { this->post_init(server); });
            });

        server.add_stop_callback(
            [&]
            {
                // The naive way to do this would be:
                //  - lock self->state
                //  - unregister observers
                //
                // This may cause a deadlock in the following case:
                //  - lock self->state
                //  - on another thread, the observer gets called into, attempts to lock self->state and blocks
                //  - unregistering the observer waits until the observer returns,
                //    which it will not since its waiting on the lock
                //  - deadlock: unregister waiting on observer, observer waiting on lock held to unregister
                //
                //  The lock is only required to grab references to the observers
                //  and handles, so we lock, copy, then unregister without holding
                //  the lock.

                Handle local_drag, local_resize, local_zoom_in, local_zoom_out;
                {
                    auto s = state.lock();

                    local_drag = std::exchange(s->drag, {});
                    local_resize = std::exchange(s->resize, {});
                    local_zoom_in = std::exchange(s->zoom_in, {});
                    local_zoom_out = std::exchange(s->zoom_out, {});
                }

                auto const cursor_mux = cursor_multiplexer.lock();
                if (cursor_mux && cursor_observer)
                    cursor_mux->unregister_interest(*cursor_observer);

                if (display_config_observer)
                    server.the_display_configuration_observer_registrar()->unregister_interest(
                        *display_config_observer);

                for (auto* handle : {&local_drag, &local_resize, &local_zoom_in, &local_zoom_out})
                    handle->reset();

                input_filter.reset();
            });
    }

    void post_init(mir::Server& server)
    {
        cursor_observer = std::make_shared<CursorObserver>(this);
        server.the_cursor_observer_multiplexer()->register_interest(cursor_observer);
        cursor_multiplexer = server.the_cursor_observer_multiplexer();

        display_config_observer = std::make_shared<DisplayConfigObserver>(*this);
        server.the_display_configuration_observer_registrar()->register_interest(display_config_observer);

        auto s = state.lock();

        if (s->visual_size == geom::Size{})
        {
            s->visual_size =
                geom::Size{default_capture_width, default_capture_height} * s->magnification;
        }

        auto const capture_compositor_id = s->render_scene_into_surface.capture_compositor_id();
        s->drag.init(server, HandleKind::drag, capture_compositor_id);
        s->resize.init(server, HandleKind::resize, capture_compositor_id);
        s->zoom_in.init(server, HandleKind::zoom_in, capture_compositor_id);
        s->zoom_out.init(server, HandleKind::zoom_out, capture_compositor_id);

        if (auto const surf = s->surface.lock(); surf && s->default_enabled)
        {
            if (!s->follow_cursor)
            {
                attach_observers(*s);
                auto const logical_rect = s->render_scene_into_surface.capture_area();
                s->apply_geometry(s->layout().freely_positioned(logical_rect.top_left));
                s->show_all_handles();
            }
            else
            {
                place_at_cursor(*s);
            }
        }
    }

    void set_enable(bool enable)
    {
        auto s = state.lock();
        s->default_enabled = enable;
        auto const surf = s->surface.lock();
        if (!surf)
            return;

        if (enable)
        {
            if (!s->follow_cursor)
            {
                // Place surface at last known cursor position when enabling
                // and not following the cursor.
                place_at_cursor(*s);
                s->show_all_handles();
            }
            surf->show();
        }
        else
        {
            surf->hide();
            s->hide_all_handles();
        }
    }

    void set_magnification(float new_magnification)
    {
        set_magnification(*state.lock(), new_magnification);
    }

    void set_magnification(State& s, float new_magnification)
    {
        if (auto const surf = s.surface.lock())
        {
            auto const old_layout = s.layout();
            auto const old_placement = s.placement_for(*surf);
            s.magnification = new_magnification;
            auto const new_layout = s.layout();

            if (!s.follow_cursor)
            {
                s.apply_geometry(
                    new_layout.freely_positioned_centered_on(old_layout.surface_center(old_placement)));
            }
            else
            {
                s.apply_geometry(new_layout.centered_on(old_layout.capture_center(old_placement)));
            }
        }
        else
        {
            s.magnification = new_magnification;
        }
    }

    void set_capture_size(geom::Size const& size)
    {
        auto s = state.lock();
        auto const layout = miral::MagnifierLayout{s->screen_bounds, size * s->magnification, s->magnification};
        s->apply_geometry(
            s->follow_cursor ?
                layout.centered_on(s->cursor_pos) :
                layout.freely_positioned_centered_on(s->cursor_pos));
    }

    geom::Size current_size() const { return state.lock()->render_scene_into_surface.capture_area().size; }

    void follow_cursor()
    {
        auto s = state.lock();
        if (s->follow_cursor)
            return;

        s->follow_cursor = true;
        for (auto* handle : {&s->drag, &s->resize, &s->zoom_in, &s->zoom_out})
        {
            handle->detach_observer();
            handle->hide();
        }

        place_at_cursor(*s);
    }

    void stop_following_cursor()
    {
        auto s = state.lock();
        if (!s->follow_cursor)
            return;

        s->follow_cursor = false;

        if (auto const surf = s->surface.lock())
        {
            attach_observers(*s);
            if (s->default_enabled)
            {
                auto const logical_rect = s->render_scene_into_surface.capture_area();
                s->apply_geometry(s->layout().freely_positioned(logical_rect.top_left));
                s->show_all_handles();
            }
        }
    }

private:
    class InputFilter : public mi::EventFilter
    {
    public:
        explicit InputFilter(mir::Synchronised<State>& state) : state{state} {}

        auto handle(MirEvent const& event) -> bool override;

    private:
        void reset_gestures();
        auto handle_pointer(MirPointerEvent const& event, State const& state) -> bool;
        auto handle_touch(MirTouchEvent const& event, State const& state) -> bool;

        mir::Synchronised<State>& state;
        std::optional<bool> pointer_gesture_consumed;
        std::optional<bool> touch_gesture_consumed;
    };

    class DisplayConfigObserver : public mg::DisplayConfigurationObserver
    {
    public:
        DisplayConfigObserver(Self& self) : self{self} {}

        void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
        { update_bounds(config); }

        void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
        { update_bounds(config); }

        void base_configuration_updated(std::shared_ptr<mg::DisplayConfiguration const> const&) override {}
        void session_configuration_applied(
            std::shared_ptr<ms::Session> const&,
            std::shared_ptr<mg::DisplayConfiguration> const&) override
        {}
        void session_configuration_removed(std::shared_ptr<ms::Session> const&) override {}
        void configuration_failed(std::shared_ptr<mg::DisplayConfiguration const> const&, std::exception const&)
            override
        {}
        void catastrophic_configuration_error(
            std::shared_ptr<mg::DisplayConfiguration const> const&,
            std::exception const&) override
        {}
        void configuration_updated_for_session(
            std::shared_ptr<ms::Session> const&,
            std::shared_ptr<mg::DisplayConfiguration const> const&) override
        {}

    private:
        void update_bounds(std::shared_ptr<mg::DisplayConfiguration const> const& config);
        Self& self;
    };

    /// Creates and registers concrete observers on each handle. Guards against
    /// missing indicators.
    void attach_observers(State& state)
    {
        state.drag.attach_observer<DragHandleObserver>(this);
        state.resize.attach_observer<ResizeDragObserver>(this);
        state.zoom_in.attach_observer<ZoomButtonObserver>(this, +zoom_step);
        state.zoom_out.attach_observer<ZoomButtonObserver>(this, -zoom_step);
    }

    /// Applies visual geometry with the magnifier's logical top-left computed
    /// from its current visual size so the surface is centred on the cursor.
    void place_at_cursor(State& s)
    {
        auto const layout = s.layout();
        s.apply_geometry(
            s.follow_cursor ?
                layout.centered_on(s.cursor_pos) :
                layout.freely_positioned_centered_on(s.cursor_pos));
    }

    /// Base for observers that turn a pointer/touch drag on a handle surface
    /// into on_drag_start()/on_drag_move() callbacks. Both callbacks are invoked
    /// with the magnifier mutex held; grab_abs holds the drag's grab position.
    class HandleObserver : public ms::NullSurfaceObserver
    {
    public:
        explicit HandleObserver(Self* self) : self(self) {}

        void input_consumed(ms::Surface const*, std::shared_ptr<MirEvent const> const& event) override
        {
            if (mir_event_get_type(event.get()) != mir_event_type_input)
                return;
            auto const* input_ev = mir_event_get_input_event(event.get());
            switch (mir_input_event_get_type(input_ev))
            {
            case mir_input_event_type_pointer:
                handle_pointer(mir_input_event_get_pointer_event(input_ev));
                break;
            case mir_input_event_type_touch:
                handle_touch(mir_input_event_get_touch_event(input_ev));
                break;
            default:
                break;
            }
        }

    protected:
        virtual void on_drag_start(State& s, geom::Point point) = 0;
        virtual void on_drag_move(State& s, geom::Point point) = 0;

        Self* self;
        bool drag_active{false};
        geom::Displacement grab_abs{};

    private:
        void begin_drag(State& s, geom::Point point)
        {
            drag_active = true;
            grab_abs = {geom::as_delta(point.x), geom::as_delta(point.y)};
            on_drag_start(s, point);
        }

        void handle_pointer(MirPointerEvent const* pev)
        {
            auto const action = mir_pointer_event_action(pev);
            auto const point = geom::Point{
                std::round(mir_pointer_event_axis_value(pev, mir_pointer_axis_x)),
                std::round(mir_pointer_event_axis_value(pev, mir_pointer_axis_y)),
            };

            auto s = self->state.lock();
            if (action == mir_pointer_action_button_down &&
                mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                begin_drag(*s, point);
            else if (
                action == mir_pointer_action_motion && drag_active &&
                mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                on_drag_move(*s, point);
            else if (action == mir_pointer_action_button_up)
                drag_active = false;
        }

        void handle_touch(MirTouchEvent const* tev)
        {
            if (mir_touch_event_point_count(tev) != 1)
                return;
            auto const action = mir_touch_event_action(tev, 0);
            auto const point = geom::Point{
                std::round(mir_touch_event_axis_value(tev, 0, mir_touch_axis_x)),
                std::round(mir_touch_event_axis_value(tev, 0, mir_touch_axis_y)),
            };

            auto s = self->state.lock();
            if (action == mir_touch_action_down)
                begin_drag(*s, point);
            else if (action == mir_touch_action_change && drag_active)
                on_drag_move(*s, point);
            else if (action == mir_touch_action_up)
                drag_active = false;
        }
    };

    /// Moves the magnifier when its drag handle is dragged.
    class DragHandleObserver : public HandleObserver
    {
    public:
        using HandleObserver::HandleObserver;

    protected:
        void on_drag_start(State& s, geom::Point) override
        {
            auto const surf = s.surface.lock();
            if (!surf)
                return;

            drag_start_magnifier_pos = surf->top_left();
        }

        void on_drag_move(State& s, geom::Point point) override
        {
            geom::Displacement const delta{
                geom::as_delta(point.x - grab_abs.dx), geom::as_delta(point.y - grab_abs.dy)};
            geom::Point const new_surface_top_left{
                drag_start_magnifier_pos.x + delta.dx, drag_start_magnifier_pos.y + delta.dy};

            s.apply_geometry(s.layout().freely_positioned(new_surface_top_left));
        }

        geom::Point drag_start_magnifier_pos{};
    };

    class CursorObserver : public mi::CursorObserver
    {
    public:
        explicit CursorObserver(Self* self) : self(self) {}

        void cursor_moved_to(float abs_x, float abs_y) override
        {
            auto s = self->state.lock();
            s->cursor_pos = geom::Point{abs_x, abs_y};

            if (!s->follow_cursor)
                return;

            auto const surf = s->surface.lock();
            if (!surf)
                return;

            self->place_at_cursor(*s);
        }

        void pointer_usable() override {}
        void pointer_unusable() override {}
        void image_set_to(std::shared_ptr<mg::CursorImage>) override {}

    private:
        Self* self;
    };

    /// Observes input on the resize handle indicator and changes the magnifier capture size.
    /// Resizes the magnifier capture area when its resize handle is dragged.
    ///
    /// Resizing model
    /// --------------
    /// The magnifier draws a *logical* capture rectangle (top-left L, size sw x sh)
    /// scaled by `mag` into a larger *visual* rectangle on screen. Both share the same
    /// centre, so (with inner = (mag-1)/2, outer = (mag+1)/2) the visual edges are:
    ///     visual left/top    = L - inner * size
    ///     visual right/bottom = L + outer * size
    ///
    /// A resize drag keeps the visual corner *opposite* the grabbed handle pinned and
    /// lets the grabbed corner follow the finger:
    ///
    ///     pin +-----------+
    ///         |  visual   |
    ///         |   rect    |
    ///         +-----------X  <- grabbed corner follows the finger (ax, ay)
    ///
    /// on_drag_start records the pinned visual corner. on_drag_move measures the visual
    /// extent from pin to finger, converts it back to a logical size (/ mag), then
    /// back-solves the surface top-left so the pinned visual corner stays put unless
    /// keeping the resized magnifier on-screen requires clamping it.
    class ResizeDragObserver : public HandleObserver
    {
    public:
        using HandleObserver::HandleObserver;

    protected:
        void on_drag_start(State& s, geom::Point) override
        {
            auto const surf = s.surface.lock();
            if (!surf)
                return;
            auto const layout = s.layout();
            pin_pos = layout.resize_anchor(s.placement_for(*surf));
        }

        void on_drag_move(State& s, geom::Point point) override
        {
            auto const surf = s.surface.lock();
            if (!surf)
                return;

            s.apply_geometry(s.layout().resized_from_pinned_corner(pin_pos, point));
        }

        geom::Point pin_pos{};
    };

    /// Adjusts the magnification level when a zoom button is tapped or touched.
    class ZoomButtonObserver : public ms::NullSurfaceObserver
    {
    public:
        ZoomButtonObserver(Self* self, float delta) : self{self}, delta{delta} {}

        void input_consumed(ms::Surface const*, std::shared_ptr<MirEvent const> const& event) override
        {
            if (mir_event_get_type(event.get()) != mir_event_type_input)
                return;
            auto const* input_ev = mir_event_get_input_event(event.get());
            switch (mir_input_event_get_type(input_ev))
            {
            case mir_input_event_type_pointer:
                handle_pointer(mir_input_event_get_pointer_event(input_ev));
                break;
            case mir_input_event_type_touch:
                handle_touch(mir_input_event_get_touch_event(input_ev));
                break;
            default:
                break;
            }
        }

    private:
        void handle_pointer(MirPointerEvent const* pev)
        {
            auto const action = mir_pointer_event_action(pev);
            auto s = self->state.lock();
            if (action == mir_pointer_action_button_down &&
                mir_pointer_event_button_state(pev, mir_pointer_button_primary))
            {
                apply_zoom(*s);
            }
        }

        void handle_touch(MirTouchEvent const* tev)
        {
            if (mir_touch_event_point_count(tev) != 1)
                return;
            auto const action = mir_touch_event_action(tev, 0);
            auto s = self->state.lock();
            if (action == mir_touch_action_down)
            {
                apply_zoom(*s);
            }
        }

        /// Clamps and applies the zoom step. Caller must hold self->state.
        void apply_zoom(State& s)
        {
            self->set_magnification(
                s, std::clamp(s.magnification + delta, min_magnification + zoom_step, max_magnification));
        }

        Self* self;
        float delta;
    };

    mir::Synchronised<State> state;

    std::shared_ptr<InputFilter> input_filter;
    std::shared_ptr<CursorObserver> cursor_observer;
    std::shared_ptr<DisplayConfigObserver> display_config_observer;
    std::weak_ptr<mi::CursorObserverMultiplexer> cursor_multiplexer;
};

auto miral::Magnifier::Self::InputFilter::handle(MirEvent const& event) -> bool
{
    if (event.type() != mir_event_type_input)
        return false;

    auto const s = state.lock();
    if (s->follow_cursor || !s->default_enabled)
    {
        reset_gestures();
        return false;
    }

    auto const* input_event = event.to_input();
    switch (input_event->input_type())
    {
    case mir_input_event_type_pointer:
        return handle_pointer(*input_event->to_pointer(), *s);
    case mir_input_event_type_touch:
        return handle_touch(*input_event->to_touch(), *s);
    default:
        return false;
    }
}

void miral::Magnifier::Self::InputFilter::reset_gestures()
{
    pointer_gesture_consumed.reset();
    touch_gesture_consumed.reset();
}

auto miral::Magnifier::Self::InputFilter::handle_pointer(MirPointerEvent const& event, State const& state) -> bool
{
    auto const action = event.action();
    if (action == mir_pointer_action_button_down && !pointer_gesture_consumed)
    {
        pointer_gesture_consumed =
            event.position()
                .transform([&state](auto const& position) { return state.point_is_on_magnifier_surface(position); })
                .value_or(false);
    }

    if (pointer_gesture_consumed)
    {
        auto const consumed = *pointer_gesture_consumed;
        if (action == mir_pointer_action_button_up && event.buttons() == 0)
            pointer_gesture_consumed.reset();
        return consumed;
    }

    return event.position()
        .transform([&state](auto const& position) { return state.point_is_on_magnifier_surface(position); })
        .value_or(false);
}

auto miral::Magnifier::Self::InputFilter::handle_touch(MirTouchEvent const& event, State const& state) -> bool
{
    if (!touch_gesture_consumed)
    {
        bool starts_on_magnifier = false;
        for (auto i = 0u; i != event.pointer_count(); ++i)
        {
            starts_on_magnifier = starts_on_magnifier || state.point_is_on_magnifier_surface(event.position(i));
        }
        touch_gesture_consumed = starts_on_magnifier;
    }

    auto const consumed = *touch_gesture_consumed;
    bool gesture_ended = event.pointer_count() > 0;
    for (auto i = 0u; i != event.pointer_count(); ++i)
        gesture_ended = gesture_ended && event.action(i) == mir_touch_action_up;

    if (gesture_ended)
        touch_gesture_consumed.reset();

    return consumed;
}

void miral::Magnifier::Self::DisplayConfigObserver::update_bounds(
    std::shared_ptr<mg::DisplayConfiguration const> const& config)
{
    geom::Rectangles rects;
    config->for_each_output(
        [&rects](mg::DisplayConfigurationOutput const& output)
        {
            if (output.used && output.connected)
                rects.add(output.extents());
        });

    auto s = self.state.lock();
    s->screen_bounds = rects;
    if (auto const surf = s->surface.lock())
    {
        auto const top_left = surf->top_left();
        auto const pre_clamp_visual_size = s->visual_size;
        auto const layout = s->layout();
        auto const placement =
            s->follow_cursor ?
                layout.centered_on(s->cursor_pos) :
                layout.freely_positioned(top_left);
        if (pre_clamp_visual_size == placement.visual_size)
            return;

        s->apply_geometry(placement);
    }
}

miral::Magnifier::Magnifier() : self(std::make_shared<Self>()) {}

miral::Magnifier::Magnifier(live_config::Store& config_store) : Magnifier()
{
    config_store.add_bool_attribute(
        {"magnifier", "enable"},
        "Whether the magnifier is enabled",
        [this](live_config::Key const&, std::optional<bool> val)
        {
            if (val.has_value())
            {
                enable(*val);
            }
        });
    config_store.add_float_attribute(
        {"magnifier", "magnification"},
        "The magnification scale ",
        default_magnification,
        [this](live_config::Key const&, std::optional<float> val)
        { magnification(val.value_or(default_magnification)); });
    config_store.add_int_attribute(
        {"magnifier", "capture_size", "width"},
        "The width of the rectangular region that will be magnified",
        default_capture_width,
        [this](live_config::Key const& key, std::optional<int> val)
        {
            if (val.has_value() && *val <= 0)
            {
                mir::log_warning("Config key '%s' should be greater than 0", key.to_string().c_str());
                return;
            }

            if (!val.has_value())
                return;

            auto size = self->current_size();
            size.width = geom::Width(*val);
            capture_size(size);
        });
    config_store.add_int_attribute(
        {"magnifier", "capture_size", "height"},
        "The height of the rectangular region that will be magnified",
        default_capture_height,
        [this](live_config::Key const& key, std::optional<int> val)
        {
            if (val.has_value() && *val <= 0)
            {
                mir::log_warning("Config key '%s' should be greater than 0", key.to_string().c_str());
                return;
            }

            if (!val.has_value())
                return;

            auto size = self->current_size();
            size.height = geom::Height(*val);
            capture_size(size);
        });
}

miral::Magnifier& miral::Magnifier::enable(bool enabled)
{
    self->set_enable(enabled);
    return *this;
}

miral::Magnifier& miral::Magnifier::magnification(float magnification)
{
    auto const clamped_magnification = std::clamp(magnification, min_magnification, max_magnification);
    if (magnification != clamped_magnification)
    {
        mir::log_warning("Magnification should be between %.2f and %.2f", min_magnification, max_magnification);

        return *this;
    }

    self->set_magnification(clamped_magnification);
    return *this;
}

miral::Magnifier& miral::Magnifier::capture_size(mir::geometry::Size const& size)
{
    self->set_capture_size(size);
    return *this;
}

miral::Magnifier& miral::Magnifier::set_behavior(Behavior behavior)
{
    switch (behavior)
    {
    case Behavior::follow_cursor:
        self->follow_cursor();
        break;
    case Behavior::freely_positioned:
        self->stop_following_cursor();
        break;
    }
    return *this;
}

void miral::Magnifier::operator()(mir::Server& server) { self->init(server); }
