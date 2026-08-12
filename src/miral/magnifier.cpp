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

#include <input-method-unstable-v2_wrapper.h>
#include <wayland_wrapper.h>

#include <miral/live_config.h>
#include <mir/events/event.h>
#include <mir/events/input_event.h>
#include <mir/events/pointer_event.h>
#include <mir/events/touch_event.h>
#include <mir/log.h>
#include <mir/server.h>
#include <mir/synchronised.h>
#include <mir/graphics/display_configuration.h>
#include <mir/graphics/display_configuration_observer.h>
#include <mir/geometry/rectangles.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/scene/surface.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/scene/basic_surface.h>
#include <mir/shell/surface_stack.h>
#include <mir/observer_registrar.h>
#include <mir/main_loop.h>

#include <algorithm>
#include <concepts>
#include <array>
#include <glm/gtc/matrix_transform.hpp>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace mc = mir::compositor;

namespace
{
auto const default_capture_width = 300;
auto const default_capture_height = 300;
/// The lowest magnification that still reads as visibly magnified; below
/// this the magnifier is hard to distinguish from the unmagnified scene.
auto const min_magnification = 1.25f;
auto const default_magnification = 1.25f;
auto const max_magnification = 8.0f;

/// Magnification step applied by each zoom button press.
auto const zoom_step = 0.25f;

class Handle
{
public:
    struct ObserverRegistration
    {
        std::shared_ptr<miral::HandleIndicator> indicator;
        std::shared_ptr<ms::NullSurfaceObserver> observer;

        void unregister()
        {
            if (observer)
                indicator->unregister_interest(*observer);
        }
    };

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
        if (!indicator || observer)
            return;

        observer = std::make_shared<ObserverType>(std::forward<Args>(args)...);
        indicator->register_interest(observer);
    }

    void detach_observer()
    {
        release_observer().unregister();
    }

    auto release_observer() -> ObserverRegistration
    {
        return {indicator, std::exchange(observer, {})};
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

/// The magnifier's control handles, named to match
/// `MagnifierLayout::HandlePositions` so the two stay in step.
struct Handles
{
    Handle drag;
    Handle resize;
    Handle zoom_in;
    Handle zoom_out;

    void for_each(std::invocable<Handle&, miral::HandleKind> auto&& f)
    {
        f(drag, miral::HandleKind::drag);
        f(resize, miral::HandleKind::resize);
        f(zoom_in, miral::HandleKind::zoom_in);
        f(zoom_out, miral::HandleKind::zoom_out);
    }
};

struct State
{
    auto layout() const -> miral::MagnifierLayout
    {
        return {screen_bounds, preferred_visual_size, magnification};
    }

    auto current_placement(ms::Surface const& surf) const -> miral::MagnifierLayout::Placement
    {
        return layout().placement_of(render_scene_into_surface.capture_area(), surf.top_left());
    }

    /// The corner the user currently sees, which is what the free-placement
    /// entry points expect. Re-placing there is a no-op that re-applies the
    /// current output confinement.
    auto visible_top_left(ms::Surface const& surf) const -> geom::Point
    {
        return current_placement(surf).visual_bounds().top_left;
    }

    void apply_geometry(miral::MagnifierLayout::Placement const& placement)
    {
        preferred_visual_size = placement.preferred_visual_size;
        render_scene_into_surface.capture_area(placement.capture_area);

        if (!follow_cursor)
        {
            auto const positions = placement.handle_positions();
            handles.for_each([&](Handle& handle, miral::HandleKind kind)
                { handle.move_to(positions.for_kind(kind)); });
        }

        if (auto const surf = surface.lock())
        {
            surf->move_to(placement.untransformed_surface_top_left);
            surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
        }
    }

    void show_all_handles() { handles.for_each([](Handle& handle, auto) { handle.show(); }); }

    void hide_all_handles() { handles.for_each([](Handle& handle, auto) { handle.hide(); }); }

    /// Places the magnifier by whichever rule the current behavior calls for.
    /// Both rules take the point the magnifier is to be centred on, so the
    /// choice between them lives here rather than at every call site.
    void place_centered_on(miral::MagnifierLayout const& layout, geom::Point center)
    {
        apply_geometry(
            follow_cursor ? layout.place_following_cursor_at(center) : layout.place_freely_centered_on(center));
    }

    /// Where a freely-positioned magnifier belongs: wherever the user last
    /// dragged or resized it to, or, until they have moved it at all, the
    /// centre of the primary output. Deliberately not the cursor, which is
    /// where the follow-cursor behavior would have left it.
    auto place_freely(ms::Surface const& surf) const -> miral::MagnifierLayout::Placement
    {
        auto const l = layout();
        return user_positioned ?
            l.place_freely_at(visible_top_left(surf)) :
            l.place_freely_centered_on(primary_output_center());
    }

    /// The centre of the primary output, i.e. the first usable one.
    auto primary_output_center() const -> geom::Point
    {
        auto const primary = std::ranges::find_if(
            screen_bounds,
            [](geom::Rectangle const& output)
            { return output.size.width > geom::Width{0} && output.size.height > geom::Height{0}; });

        if (primary == std::ranges::end(screen_bounds))
            return {};

        return primary->top_left +
               geom::Displacement{
                   geom::as_delta(primary->size.width / 2), geom::as_delta(primary->size.height / 2)};
    }

    std::weak_ptr<ms::Surface> surface;
    Handles handles;
    geom::Point cursor_pos;
    geom::Rectangles screen_bounds;
    float magnification{default_magnification};
    /// The on-screen size the user has asked the magnifier to be. Persistent
    /// intent: when the magnification changes the capture size is recomputed
    /// from this so the magnifier's on-screen footprint stays the same.
    geom::Size preferred_visual_size;
    bool enabled{false};
    bool follow_cursor{true};
    /// Whether the user has dragged or resized the magnifier. Until they
    /// have, it keeps returning to the centre of the primary output.
    bool user_positioned{false};
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

                if (s->enabled)
                    surf->show();
                else
                    surf->hide();

                s->surface = surf;
            });

        s->render_scene_into_surface(server);

        server.add_init_callback(
            [&]
            {
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

                Handles local_handles;
                {
                    auto s = state.lock();

                    local_handles = std::exchange(s->handles, {});
                }

                auto const cursor_mux = cursor_multiplexer.lock();
                if (cursor_mux && cursor_observer)
                    cursor_mux->unregister_interest(*cursor_observer);

                if (display_config_observer)
                    server.the_display_configuration_observer_registrar()->unregister_interest(
                        *display_config_observer);

                local_handles.for_each([](Handle& handle, auto) { handle.reset(); });

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

        if (s->preferred_visual_size == geom::Size{})
        {
            s->preferred_visual_size =
                geom::Size{default_capture_width, default_capture_height} * s->magnification;
        }

        auto const capture_compositor_id = s->render_scene_into_surface.capture_compositor_id();
        s->handles.for_each([&](Handle& handle, miral::HandleKind kind)
            { handle.init(server, kind, capture_compositor_id); });

        if (!s->follow_cursor)
            attach_observers(*s);

        if (auto const surf = s->surface.lock(); surf && s->enabled)
        {
            if (!s->follow_cursor)
            {
                s->apply_geometry(s->place_freely(*surf));
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
        s->enabled = enable;
        auto const surf = s->surface.lock();
        if (!surf)
            return;

        if (enable)
        {
            if (!s->follow_cursor)
            {
                attach_observers(*s);
                s->apply_geometry(s->place_freely(*surf));
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
            auto const old_placement = s.current_placement(*surf);
            s.magnification = new_magnification;
            s.place_centered_on(s.layout(), old_placement.scaling_center());
        }
        else
        {
            s.magnification = new_magnification;
        }
    }

    void set_capture_size(geom::Size const& size)
    {
        auto s = state.lock();
        s->place_centered_on(
            miral::MagnifierLayout{s->screen_bounds, size * s->magnification, s->magnification}, s->cursor_pos);
    }

    geom::Size current_size() const { return state.lock()->render_scene_into_surface.capture_area().size; }

    void follow_cursor()
    {
        if (state.lock()->follow_cursor)
            return;

        auto observers = [this]()
        {
            auto s = state.lock();
            s->follow_cursor = true;

            std::array<Handle::ObserverRegistration, 4> const ret{
                s->handles.drag.release_observer(),
                s->handles.resize.release_observer(),
                s->handles.zoom_in.release_observer(),
                s->handles.zoom_out.release_observer()};

            s->hide_all_handles();
            place_at_cursor(*s);

            return ret;
        }();

        for (auto obs : observers)
            obs.unregister();
    }

    void stop_following_cursor()
    {
        auto s = state.lock();
        if (!s->follow_cursor)
            return;

        s->follow_cursor = false;
        attach_observers(*s);

        // Following the cursor leaves the magnifier sitting on it, which is
        // exactly where a freely-positioned magnifier should not start.
        s->user_positioned = false;

        if (auto const surf = s->surface.lock(); surf && s->enabled)
        {
            s->apply_geometry(s->place_freely(*surf));
            s->show_all_handles();
        }
    }

private:
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
        state.handles.drag.attach_observer<DragHandleObserver>(this);
        state.handles.resize.attach_observer<ResizeDragObserver>(this);
        state.handles.zoom_in.attach_observer<ZoomButtonObserver>(this, +zoom_step);
        state.handles.zoom_out.attach_observer<ZoomButtonObserver>(this, -zoom_step);
    }

    /// Applies visual geometry with the magnifier's logical top-left computed
    /// from its current visual size so the surface is centred on the cursor.
    void place_at_cursor(State& s) { s.place_centered_on(s.layout(), s.cursor_pos); }

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

            drag_start_visual_top_left = s.visible_top_left(*surf);
        }

        void on_drag_move(State& s, geom::Point point) override
        {
            geom::Displacement const delta{
                geom::as_delta(point.x - grab_abs.dx), geom::as_delta(point.y - grab_abs.dy)};
            geom::Point const new_visual_top_left{
                drag_start_visual_top_left.x + delta.dx, drag_start_visual_top_left.y + delta.dy};

            s.user_positioned = true;
            s.apply_geometry(s.layout().place_freely_at(new_visual_top_left));
        }

        geom::Point drag_start_visual_top_left{};
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
            pinned_visual_corner = s.current_placement(*surf).resize_anchor();
        }

        void on_drag_move(State& s, geom::Point point) override
        {
            auto const surf = s.surface.lock();
            if (!surf)
                return;

            s.user_positioned = true;
            s.apply_geometry(s.layout().resize_from_pinned_corner(pinned_visual_corner, point));
        }

        geom::Point pinned_visual_corner{};
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
                s, std::clamp(s.magnification + delta, min_magnification, max_magnification));
        }

        Self* self;
        float delta;
    };

    mir::Synchronised<State> state;

    std::shared_ptr<CursorObserver> cursor_observer;
    std::shared_ptr<DisplayConfigObserver> display_config_observer;
    std::weak_ptr<mi::CursorObserverMultiplexer> cursor_multiplexer;
};

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
        // The new bounds reach the placement through State::layout(), which
        // reads screen_bounds: re-placing therefore re-clamps the magnifier's
        // size and re-confines it to whichever output it now sits on.
        auto const placement =
            s->follow_cursor ? s->layout().place_following_cursor_at(s->cursor_pos) : s->place_freely(*surf);
        if (placement != s->current_placement(*surf))
            s->apply_geometry(placement);
    }
}

miral::Magnifier::Magnifier()
    : self(std::make_shared<Self>())
{
}

miral::Magnifier::Magnifier(live_config::Store& config_store)
    : Magnifier()
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
                mir::log_warning(
                    "Config key '%s' should be greater than 0",
                    key.to_string().c_str());
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
                mir::log_warning(
                    "Config key '%s' should be greater than 0",
                    key.to_string().c_str());
                return;
            }

            if (!val.has_value())
                return;

            auto size = self->current_size();
            size.height = geom::Height(*val);
            capture_size(size);
        });
    config_store.add_string_attribute(
        {"magnifier", "behavior"},
        "What behavior the magnifier should exhibit",
        [this](live_config::Key const&, std::optional<std::string_view> val)
        {
            if (!val.has_value())
                return;

            if (*val == "follow_cursor")
                set_behavior(Behavior::follow_cursor);
            else if (*val == "freely_positioned")
                set_behavior(Behavior::freely_positioned);
            else
                mir::log_warning(
                    "Config key 'magnifier.behavior' should be either 'follow_cursor' or 'freely_positioned'");
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

void miral::Magnifier::operator()(mir::Server& server)
{
    self->init(server);
}
