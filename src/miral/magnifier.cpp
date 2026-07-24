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

#include "render_scene_into_surface.h"
#include "software_buffer_pool.h"

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
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/shell/surface_stack.h>
#include <mir/observer_registrar.h>
#include <mir/main_loop.h>

#include <glm/gtc/matrix_transform.hpp>

#include <span>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace msh = mir::shell;
namespace mc = mir::compositor;

namespace
{
auto const default_capture_width = 150;
auto const default_capture_height = 150;
auto const min_magnification = 1.0f;
auto const default_magnification = 1.25f;
auto const max_magnification = 8.0f;

/// Diameter in pixels of the circular drag handle shown at the corner of the magnifier.
auto const drag_handle_diameter = 48;

/// Vertical gap between the zoom-in and zoom-out buttons in their stack.
auto const zoom_stack_padding = 8;

/// Height of the vertical zoom-in/zoom-out button stack.
geom::Height const zoom_stack_height{2 * drag_handle_diameter + zoom_stack_padding};

/// Magnification step applied by each zoom button press.
auto const zoom_step = 0.25f;

enum class HandleKind { drag, resize, zoom_in, zoom_out };

static std::string name_for_kind(HandleKind kind)
{
    switch (kind)
    {
    case HandleKind::drag:     return "magnifier-drag-handle";
    case HandleKind::resize:   return "magnifier-resize-handle";
    case HandleKind::zoom_in:  return "magnifier-zoom-in-handle";
    case HandleKind::zoom_out: return "magnifier-zoom-out-handle";
    }

    std::unreachable();
}

class HandleIndicator : public ms::BasicSurface
{
public:
    HandleIndicator(
        geom::Rectangle const& initial_rect,
        HandleKind kind,
        mc::CompositorID capture_compositor_id,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<ms::SceneReport> const& scene_report,
        std::shared_ptr<mir::ObserverRegistrar<mg::DisplayConfigurationObserver>> const& display_config_registrar) :
        BasicSurface{
            name_for_kind(kind),
            initial_rect,
            mir_pointer_unconfined,
            miral::create_stream_info(),
            nullptr,
            scene_report,
            display_config_registrar},
        pool{
            [allocator, initial_rect]
            { return allocator->alloc_software_buffer(initial_rect.size, mir_pixel_format_argb_8888); },
            1},
        capture_compositor_id{capture_compositor_id}
    {
        auto buffer = pool.claim();
        switch (kind)
        {
        case HandleKind::drag:
            fill_drag_buffer(*buffer);
            break;
        case HandleKind::resize:
            fill_resize_buffer(*buffer);
            break;
        case HandleKind::zoom_in:
        case HandleKind::zoom_out:
            fill_zoom_buffer(*buffer, kind == HandleKind::zoom_in);
            break;
        }
        auto const sz = window_size();
        get_streams().begin()->stream->submit_buffer(buffer, sz, geom::RectangleD{{0, 0}, sz});

        set_depth_layer(mir_depth_layer_always_on_top);
        set_focus_mode(mir_focus_mode_disabled);
        hide();
    }

    auto generate_renderables(mc::CompositorID id) const -> mg::RenderableList override
    {
        if (id == capture_compositor_id)
            return {};
        return BasicSurface::generate_renderables(id);
    }

private:
    class BufferPainter
    {
    public:
        struct HorizontalLine
        {
            geom::X start;
            geom::X end;
            geom::Y y;
            geom::Height thickness;
        };

        struct VerticalLine
        {
            geom::X x;
            geom::Y start;
            geom::Y end;
            geom::Width thickness;
        };

        explicit BufferPainter(mg::Buffer& buffer) :
            writable{mrs::as_write_mappable(std::shared_ptr<mg::Buffer>(&buffer, [](mg::Buffer*) {}))},
            mapping{writable->map_writeable()},
            stride{mapping->stride()},
            size{buffer.size()}
        {
            assert(mapping->format() == format);
        }

        void clear()
        {
            std::memset(mapping->data(), 0, mapping->len());
        }

        void fill_circle()
        {
            auto const cx = geom::as_x((size.width - geom::DeltaX{1}) / 2.0f);
            auto const cy = geom::as_y((size.height - geom::DeltaY{1}) / 2.0f);
            auto const r = std::min(cx.as_value(), cy.as_value());
            auto const r_sq = r * r;

            for (geom::Y y{0}; y < geom::as_y(size.height); y = y + geom::DeltaY{1})
            {
                for (geom::X x{0}; x < geom::as_x(size.width); x = x + geom::DeltaX{1})
                {
                    auto const dx = x - cx;
                    auto const dy = y - cy;
                    auto const delta = dx.as_value() * dx.as_value() + dy.as_value() * dy.as_value();

                    if (delta <= r_sq)
                        set_pixel(geom::Point{x, y}, disc_grey);
                }
            }
        }

        void draw_horizontal_line(HorizontalLine const& line)
        {
            auto const true_start = std::min<geom::X>(line.start, line.end);
            auto const true_end = std::max<geom::X>(line.start, line.end);
            for (int t = 0; t < line.thickness.as_value(); ++t)
                for (auto x = true_start; x <= true_end; x = x + geom::DeltaX{1})
                    set_pixel(geom::Point{x, line.y + geom::DeltaY{t}}, icon_grey);
        }

        void draw_vertical_line(VerticalLine const& line)
        {
            auto const true_start = std::min<geom::Y>(line.start, line.end);
            auto const true_end = std::max<geom::Y>(line.start, line.end);
            for (int t = 0; t < line.thickness.as_value(); ++t)
                for (auto y = true_start; y <= true_end; y = y + geom::DeltaY{1})
                    set_pixel(geom::Point{line.x + geom::DeltaX{t}, y}, icon_grey);
        }

        auto buffer_size() const -> geom::Size
        {
            return size;
        }

    private:
        void set_pixel(geom::Point const& point, uint8_t grey)
        {
            auto const x = point.x.as_value();
            auto const y = point.y.as_value();
            if (x < 0 || x >= size.width.as_value() || y < 0 || y >= size.height.as_value())
                return;

            // For mir_pixel_format_argb_8888 on little-endian: memory layout [B, G, R, A].
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            std::span<std::uint8_t> const pixels{reinterpret_cast<std::uint8_t*>(mapping->data()), mapping->len()};
            auto const offset = y * stride.as_value() + x * MIR_BYTES_PER_PIXEL(format);
            auto const pixel = pixels.subspan(offset, MIR_BYTES_PER_PIXEL(format));
            pixel[0] = pixel[1] = pixel[2] = grey;
            pixel[3] = background_alpha;
        }

        /// Painter colour/alpha constants for handle indicator graphics.
        inline uint8_t static disc_grey = 40;
        inline uint8_t static icon_grey = 210;
        inline uint8_t static background_alpha = 150;

        static auto constexpr format = mir_pixel_format_argb_8888;
        std::shared_ptr<mrs::WriteMappable> const writable;
        std::unique_ptr<mrs::Mapping<std::byte>> const mapping;
        geom::Stride const stride;
        geom::Size const size;
    };

    static void fill_drag_buffer(mg::Buffer& buffer)
    {
        BufferPainter painter{buffer};
        painter.clear();
        painter.fill_circle();

        // Draw a drag-pan icon: four arrowheads (N/S/E/W) connected by slim stems.
        static int constexpr tip_dist  = 15;
        static int constexpr head_len  = 5;
        static int constexpr stem_thickness = 3;
        static int constexpr base_dist = (tip_dist - head_len);

        auto const [w, h]  = painter.buffer_size();
        auto const center_x = geom::as_x((w - geom::DeltaX{1}) / 2);
        auto const center_y = geom::as_y((h - geom::DeltaY{1}) / 2);
        auto const tip_n  = center_y - geom::DeltaY{tip_dist};
        auto const tip_s  = center_y + geom::DeltaY{tip_dist};
        auto const tip_w  = center_x - geom::DeltaX{tip_dist};
        auto const tip_e  = center_x + geom::DeltaX{tip_dist};
        auto const base_n = center_y - geom::DeltaY{base_dist};
        auto const base_s = center_y + geom::DeltaY{base_dist};
        auto const base_w = center_x - geom::DeltaX{base_dist};
        auto const base_e = center_x + geom::DeltaX{base_dist};

        // Stems
        painter.draw_vertical_line(
            {
                .x = center_x - geom::DeltaX{stem_thickness / 2},
                .start = base_n,
                .end = base_s,
                .thickness = geom::Width{stem_thickness},
            });
        painter.draw_horizontal_line(
            {
                .start = base_w,
                .end = base_e,
                .y = center_y - geom::DeltaY{stem_thickness / 2},
                .thickness = geom::Height{stem_thickness},
            });

        // North arrowhead — rows from tip_n+1 to base_n, width grows by 1 px per row
        for (auto y = tip_n + geom::DeltaY{1}; y <= base_n; y = y + geom::DeltaY{1})
            painter.draw_horizontal_line(
                {
                    .start = center_x - geom::DeltaX((y - tip_n).as_value()),
                    .end = center_x + geom::DeltaX((y - tip_n).as_value()),
                    .y = y,
                    .thickness = geom::Height{1},
                });

        // South arrowhead
        for (auto y = base_s; y < tip_s; y = y + geom::DeltaY{1})
            painter.draw_horizontal_line(
                {
                    .start = center_x - geom::DeltaX((tip_s - y).as_value()),
                    .end = center_x + geom::DeltaX((tip_s - y).as_value()),
                    .y = y,
                    .thickness = geom::Height{1},
                });

        // West arrowhead — columns from tip_w+1 to base_w
        for (auto x = tip_w + geom::DeltaX{1}; x <= base_w; x = x + geom::DeltaX{1})
            painter.draw_vertical_line(
                {
                    .x = x,
                    .start = center_y - geom::DeltaY((x - tip_w).as_value()),
                    .end = center_y + geom::DeltaY((x - tip_w).as_value()),
                    .thickness = geom::Width{1},
                });

        // East arrowhead
        for (auto x = base_e; x < tip_e; x = x + geom::DeltaX{1})
            painter.draw_vertical_line(
                {
                    .x = x,
                    .start = center_y - geom::DeltaY((tip_e - x).as_value()),
                    .end = center_y + geom::DeltaY((tip_e - x).as_value()),
                    .thickness = geom::Width{1},
                });
    }

    static void fill_resize_buffer(mg::Buffer& buffer)
    {
        BufferPainter painter{buffer};
        painter.clear();
        painter.fill_circle();

        // Corner bracket: two 2-px arms meeting at the top-left corner, which is
        // where the resize handle always sits within the magnifier surface.
        static int constexpr arm_len   = 12;
        static int constexpr thickness = 2;
        auto const [w, h] = painter.buffer_size();
        auto const corner_x =  geom::as_x(w / 4);
        auto const corner_y =  geom::as_y(h / 4);

        painter.draw_horizontal_line(
            {
                .start = corner_x,
                .end = corner_x + geom::DeltaX{arm_len},
                .y = corner_y,
                .thickness = geom::Height{thickness},
            });
        painter.draw_vertical_line(
            {
                .x = corner_x,
                .start = corner_y,
                .end = corner_y + geom::DeltaY{arm_len},
                .thickness = geom::Width{thickness},
            });
    }

    static void fill_zoom_buffer(mg::Buffer& buffer, bool zoom_in)
    {
        BufferPainter painter{buffer};
        painter.clear();
        painter.fill_circle();

        // Draw + (zoom in) or – (zoom out) symbol.
        auto const [w, h] = painter.buffer_size();
        int const bar_thickness = std::max(geom::Width{2}, w / 12).as_value();
        int const bar_len = (w * 5 / 8).as_value();

        painter.draw_horizontal_line(
            {
                .start = geom::as_x((w - geom::Width{bar_len}) / 2),
                .end = geom::as_x((w + geom::Width{bar_len}) / 2 - geom::DeltaX{1}),
                .y = geom::as_y((h - geom::Height{bar_thickness}) / 2),
                .thickness = geom::Height{bar_thickness},
            });

        if (zoom_in)
            painter.draw_vertical_line(
                {
                    .x = geom::as_x((w - geom::Width{bar_thickness}) / 2),
                    .start = geom::as_y((h - geom::Height{bar_len}) / 2),
                    .end = geom::as_y((h + geom::Height{bar_len}) / 2 - geom::DeltaY{1}),
                    .thickness = geom::Width{bar_thickness},
                });
    }

    miral::SoftwareBufferPool mutable pool;
    mc::CompositorID capture_compositor_id;
};

class Handle
{
public:
    Handle() = default;

    void init(mir::Server& server, HandleKind kind, mc::CompositorID capture_compositor_id)
    {
        indicator = std::make_shared<HandleIndicator>(
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
        {geom::Width{drag_handle_diameter}, geom::Height{drag_handle_diameter}}};

    std::shared_ptr<HandleIndicator> indicator;
    std::weak_ptr<msh::SurfaceStack> handle_surface_stack;
    std::shared_ptr<ms::NullSurfaceObserver> observer;
};

geom::Point surface_top_left_from_cursor(geom::Size const& window_size, geom::Point const& cursor_pos)
{
    return cursor_pos -
        geom::Displacement{
            geom::as_delta(window_size.width / 2),
            geom::as_delta(window_size.height / 2)};
}

geom::Size logical_size_from_visual(geom::Size const& visual_size, float mag)
{
    float widthf{static_cast<float>(visual_size.width.as_value())};
    float heightf{static_cast<float>(visual_size.height.as_value())};
    return {
        geom::Width{std::max(1.0f, std::round(widthf/ mag))},
        geom::Height{std::max(1.0f, std::round(heightf / mag))}};
}

/// Clamps a visual (on-screen) size so neither dimension shrinks below
/// min_visual_dimension.
geom::Size clamp_visual_size(geom::Size const& visual_size)
{
    /// Minimum on-screen (visual) width/height of the magnifier surface, in pixels.
    /// Derived so the drag handle (bottom-right corner) and the zoom button stack
    /// (right edge, vertically centred) never overlap each other: the drag handle
    /// needs a full diameter of clearance both above and below the centred stack.
    auto const min_visual_dimension = 2 * drag_handle_diameter + zoom_stack_height.as_int();

    return {
        std::max(visual_size.width, geom::Width{min_visual_dimension}),
        std::max(visual_size.height, geom::Height{min_visual_dimension})};
}

/// The centre of a logical capture rectangle. The visual magnifier shares this
/// centre, so it also fixes the away direction for handle placement.
geom::Point center(geom::Point const& top_left, geom::Size const& size)
{
    return top_left +
        geom::Displacement{
            geom::as_delta(size.width / 2),
            geom::as_delta(size.height / 2)};
}
}

class miral::Magnifier::Self
{
private:
    struct State;
    class Observer;

    class DisplayConfigObserver : public mg::DisplayConfigurationObserver
    {
    public:
        explicit DisplayConfigObserver(mir::Synchronised<State>& state) : state{state} {}

        void initial_configuration(
            std::shared_ptr<mg::DisplayConfiguration const> const& config) override
        {
            update_bounds(config);
        }

        void configuration_applied(
            std::shared_ptr<mg::DisplayConfiguration const> const& config) override
        {
            update_bounds(config);
        }

        void base_configuration_updated(
            std::shared_ptr<mg::DisplayConfiguration const> const&) override {}
        void session_configuration_applied(
            std::shared_ptr<ms::Session> const&,
            std::shared_ptr<mg::DisplayConfiguration> const&) override {}
        void session_configuration_removed(std::shared_ptr<ms::Session> const&) override {}
        void configuration_failed(
            std::shared_ptr<mg::DisplayConfiguration const> const&,
            std::exception const&) override {}
        void catastrophic_configuration_error(
            std::shared_ptr<mg::DisplayConfiguration const> const&,
            std::exception const&) override {}
        void configuration_updated_for_session(
            std::shared_ptr<ms::Session> const&,
            std::shared_ptr<mg::DisplayConfiguration const> const&) override {}

    private:
        void update_bounds(std::shared_ptr<mg::DisplayConfiguration const> const& config);

        mir::Synchronised<State>& state;
    };

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

public:
    Self()
    {
        render_scene_into_surface
            .capture_area(geom::Rectangle{{300, 300}, geom::Size(default_capture_width, default_capture_height)})
            .overlay_cursor(false);
    }

    void init(mir::Server& server)
    {
        render_scene_into_surface.on_surface_ready(
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

        render_scene_into_surface(server);

        server.add_init_callback([&]
        {
            input_filter = std::make_shared<InputFilter>(state);
            server.the_composite_event_filter()->prepend(input_filter);

            server.the_main_loop()->spawn(
                [=, this, &server]
                {
                    auto local_observer = std::make_shared<Observer>(this);
                    auto local_display_config_observer = std::make_shared<DisplayConfigObserver>(state);

                    // Init `State` references under lock
                    this->post_init(server, local_observer, local_display_config_observer);

                    // Keep screen bounds up to date to respond to display
                    // (re)configuration, e.g. resizing a nested window.
                    //
                    // Note: register_interest may call initial_configuration()
                    // synchronously, which calls update_bounds(), which locks
                    // state — so we must not hold the state lock across this
                    // call. Hence why we're calling after `post_init()`.
                    server.the_display_configuration_observer_registrar()->register_interest(
                        local_display_config_observer);
                    server.the_cursor_observer_multiplexer()->register_interest(local_observer);
                });
        });

        server.add_stop_callback([&]
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

            std::shared_ptr<mi::CursorObserverMultiplexer> local_cursor_mux;
            std::shared_ptr<Observer> local_observer;
            std::shared_ptr<DisplayConfigObserver> local_display_config_observer;
            Handle local_drag, local_resize, local_zoom_in, local_zoom_out;
            {
                auto s = state.lock();

                local_cursor_mux = s->cursor_multiplexer.lock();
                local_observer = s->observer;
                local_display_config_observer = std::exchange(s->display_config_observer, {});
                local_drag = std::exchange(s->drag, {});
                local_resize = std::exchange(s->resize, {});
                local_zoom_in = std::exchange(s->zoom_in, {});
                local_zoom_out = std::exchange(s->zoom_out, {});
            }

            if (local_cursor_mux && local_observer)
                local_cursor_mux->unregister_interest(*local_observer);

            if (local_display_config_observer)
            {
                server.the_display_configuration_observer_registrar()->unregister_interest(
                    *local_display_config_observer);
            }

            for (auto* handle : {&local_drag, &local_resize, &local_zoom_in, &local_zoom_out})
                handle->reset();

            input_filter.reset();
        });
    }

    void post_init(
        mir::Server& server,
        std::shared_ptr<Observer> const& local_observer,
        std::shared_ptr<DisplayConfigObserver> const& local_display_config_observer)
    {
        auto s = state.lock();

        s->observer = local_observer;
        s->display_config_observer = local_display_config_observer;
        s->cursor_multiplexer = server.the_cursor_observer_multiplexer();
        if (s->visual_size == geom::Size{})
        {
            s->visual_size =
                clamp_visual_size(geom::Size{default_capture_width, default_capture_height} * s->magnification);
        }

        auto const capture_compositor_id = render_scene_into_surface.capture_compositor_id();
        s->drag.init(server, HandleKind::drag, capture_compositor_id);
        s->resize.init(server, HandleKind::resize, capture_compositor_id);
        s->zoom_in.init(server, HandleKind::zoom_in, capture_compositor_id);
        s->zoom_out.init(server, HandleKind::zoom_out, capture_compositor_id);

        if (auto const surf = s->surface.lock(); surf && s->default_enabled)
        {
            if (!s->follow_cursor)
            {
                s->attach_observers(this);
                auto const logical_rect = render_scene_into_surface.capture_area();
                apply_positioned_geometry(surf, logical_rect.top_left, *s, render_scene_into_surface);
                s->show_all_handles();
            }
            else
            {
                place_at_cursor(surf, *s, render_scene_into_surface);
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
                place_at_cursor(surf, *s, render_scene_into_surface);
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
        auto s = state.lock();
        s->set_magnification(new_magnification, render_scene_into_surface);
    }

    void set_capture_size(geom::Size const& size)
    {
        auto s = state.lock();
        s->visual_size = clamp_visual_size(size * s->magnification);
        auto const logical_size = logical_size_from_visual(s->visual_size, s->magnification);
        auto const logical_top_left = surface_top_left_from_cursor(logical_size, s->cursor_pos);

        if (auto const surf = s->surface.lock())
            apply_positioned_geometry(surf, logical_top_left, *s, render_scene_into_surface);
    }

    geom::Size current_size() const
    {
        return render_scene_into_surface.capture_area().size;
    }

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

        if (auto const surf = s->surface.lock(); surf && s->default_enabled)
        {
            place_at_cursor(surf, *s, render_scene_into_surface);
        }
    }

    void stop_following_cursor()
    {
        auto s = state.lock();
        if (!s->follow_cursor)
            return;

        s->follow_cursor = false;

        if (auto const surf = s->surface.lock())
        {
            s->attach_observers(this);
            if (s->default_enabled)
            {
                auto const logical_rect = render_scene_into_surface.capture_area();
                apply_positioned_geometry(surf, logical_rect.top_left, *s, render_scene_into_surface);
                s->show_all_handles();
            }
        }
    }

private:
    struct State
    {
        /// Pixel-space rectangle of signed integer coordinates.
        struct VisualBounds
        {
            geom::X x;
            geom::Y y;
            geom::Width w;
            geom::Height h;
        };

        ///
        /// The visual rectangle is `mag` times larger than the logical one but
        /// shares the same centre, which is why the top-left is pulled back by
        /// (mag-1)/2 * size.
        VisualBounds compute_visual_bounds(
            geom::Point const& logical_top_left,
            geom::Size const& logical_size,
            float mag) const
        {
            auto const w = mag * logical_size.width;
            auto const h = mag * logical_size.height;
            auto const x = logical_top_left.x - geom::as_delta((mag - 1.0f) / 2.0f * logical_size.width);
            auto const y = logical_top_left.y - geom::as_delta((mag - 1.0f) / 2.0f * logical_size.height);
            return {x, y, w, h};
        }

        auto point_is_on_magnifier_surface(geom::PointF const& point) const -> bool
        {
            if (follow_cursor || !default_enabled)
                return false;

            auto const surf = surface.lock();
            if (!surf)
                return false;

            auto const surface_top_left = surf->top_left();
            auto const logical_size = surf->window_size();
            auto const visual = compute_visual_bounds(surface_top_left, logical_size, magnification);
            auto const contains = [&point](geom::Rectangle const& area)
            {
                auto const left = static_cast<float>(area.left().as_value());
                auto const right = static_cast<float>(area.right().as_value());
                auto const top = static_cast<float>(area.top().as_value());
                auto const bottom = static_cast<float>(area.bottom().as_value());
                return point.x.as_value() >= left && point.x.as_value() < right && point.y.as_value() >= top &&
                       point.y.as_value() < bottom;
            };

            if (!contains(geom::Rectangle{{visual.x, visual.y}, {visual.w, visual.h}}))
                return false;

            auto const [drag_position, resize_position] =
                compute_both_handle_positions(surface_top_left, logical_size, magnification);
            auto const [zoom_in_position, zoom_out_position] =
                compute_zoom_button_positions(surface_top_left, logical_size, magnification);

            for (auto const& handle_position :
                {drag_position, resize_position, zoom_in_position, zoom_out_position})
            {
                if (contains(geom::Rectangle{
                    handle_position,
                    {drag_handle_diameter, drag_handle_diameter}}))
                {
                    return false;
                }
            }

            return true;
        }

        /// Translates logical_top_left, if necessary, so the resulting visual
        /// rect fits within screen_bounds (preferring to keep the top-left edge
        /// on-screen if the rect is bigger than the screen in some dimension).
        /// Used when placing the magnifier so its handles don't end up
        /// off-screen and unreachable, e.g. after stopping following the
        /// cursor while it was near a screen edge. Returns logical_top_left
        /// unchanged if screen bounds aren't known yet.
        geom::Point clamp_position_to_screen(
            geom::Point const& logical_top_left,
            geom::Size const& logical_size,
            float mag) const
        {
            if (screen_bounds.size == geom::Size{})
                return logical_top_left;

            auto const [vis_x, vis_y, vis_w, vis_h] = compute_visual_bounds(logical_top_left, logical_size, mag);
            auto const screen_left = screen_bounds.top_left.x;
            auto const screen_top = screen_bounds.top_left.y;
            auto const screen_right = screen_left + geom::as_delta(screen_bounds.size.width);
            auto const screen_bottom = screen_top + geom::as_delta(screen_bounds.size.height);

            auto dx = geom::DeltaX{0};
            if (vis_x < screen_left)
                dx = screen_left - vis_x;
            else if (vis_x + geom::as_delta(vis_w) > screen_right)
                dx = screen_right - (vis_x + geom::as_delta(vis_w));

            auto dy = geom::DeltaY{0};
            if (vis_y < screen_top)
                dy = screen_top - vis_y;
            else if (vis_y + geom::as_delta(vis_h) > screen_bottom)
                dy = screen_bottom - (vis_y + geom::as_delta(vis_h));

            return logical_top_left + geom::Displacement{geom::DeltaX{dx}, geom::DeltaY{dy}};
        }

        /// Maps the visual magnifier's position across the screen to the logical
        /// capture. At a flush visual edge, the capture extends far enough outside
        /// the screen to place that screen edge just beyond the controls. Movement
        /// between the two edges is proportional.
        geom::Point capture_position_for_surface(
            geom::Point const& surface_top_left,
            geom::Size const& logical_size,
            float mag) const
        {
            if (screen_bounds.size == geom::Size{})
                return surface_top_left;

            auto const visual = compute_visual_bounds(surface_top_left, logical_size, mag);
            auto const screen_left = screen_bounds.top_left.x;
            auto const screen_top = screen_bounds.top_left.y;
            auto const screen_width = screen_bounds.size.width;
            auto const screen_height = screen_bounds.size.height;
            auto const handle_clearance = static_cast<int>(std::ceil(drag_handle_diameter / mag));

            struct AxisMapping
            {
                int visual_start;
                int visual_extent;
                int logical_extent;
                int screen_start;
                int screen_extent;
                int max_out_of_bounds;
            };

            auto const map_axis = [](AxisMapping const& axis)
            {
                int const visual_travel = axis.screen_extent - axis.visual_extent;

                if (visual_travel <= 0)
                    return axis.screen_start + (axis.screen_extent - axis.logical_extent) / 2;

                auto const progress =
                    std::clamp(static_cast<double>(axis.visual_start - axis.screen_start) / visual_travel, 0.0, 1.0);
                int const out_of_bounds = std::min(axis.logical_extent / 2, axis.max_out_of_bounds);
                int const capture_start = axis.screen_start - out_of_bounds;
                int const capture_travel = axis.screen_extent - axis.logical_extent + 2 * out_of_bounds;
                return capture_start + static_cast<int>(std::round(progress * capture_travel));
            };

            return geom::Point{
                map_axis({
                    .visual_start = visual.x.as_value(),
                    .visual_extent = visual.w.as_value(),
                    .logical_extent = logical_size.width.as_value(),
                    .screen_start = screen_left.as_value(),
                    .screen_extent = screen_width.as_value(),
                    .max_out_of_bounds = handle_clearance,
                }),
                map_axis({
                    .visual_start = visual.y.as_value(),
                    .visual_extent = visual.h.as_value(),
                    .logical_extent = logical_size.height.as_value(),
                    .screen_start = screen_top.as_value(),
                    .screen_extent = screen_height.as_value(),
                    .max_out_of_bounds = handle_clearance,
                })};
        }

        /// Computes the position of the circular drag handle indicator, inset from
        /// the bottom-right corner of the visual magnifier so it sits within the
        /// surface's own bounds.
        geom::Point compute_drag_handle_position(
            geom::Point const& logical_top_left,
            geom::Size const& logical_size,
            float mag) const
        {
            auto const [vis_x, vis_y, vis_w, vis_h] = compute_visual_bounds(logical_top_left, logical_size, mag);

            auto hx = vis_x + geom::as_delta(vis_w) - geom::DeltaX{drag_handle_diameter};
            auto hy = vis_y + geom::as_delta(vis_h) - geom::DeltaY{drag_handle_diameter};

            return geom::Point{hx, hy};
        }

        /// Computes the position of the circular resize handle indicator.
        ///
        /// The handle is placed at the visual top-left corner of the magnifier.
        geom::Point compute_resize_handle_position(
            geom::Point const& logical_top_left,
            geom::Size const& logical_size,
            float mag) const
        {
            auto const [vis_x, vis_y, vis_w, vis_h] = compute_visual_bounds(logical_top_left, logical_size, mag);

            // Default: circle centred on the top-left corner of the visual magnifier,
            // so it sits half outside the content area.
            return geom::Point{vis_x, vis_y};
        }

        std::pair<geom::Point, geom::Point> compute_both_handle_positions(
            geom::Point const& logical_top_left,
            geom::Size const& logical_size,
            float mag) const
        {
            return {
                compute_drag_handle_position(logical_top_left, logical_size, mag),
                compute_resize_handle_position(logical_top_left, logical_size, mag)};
        }

        /// Computes positions for the zoom-in and zoom-out button indicators.
        ///
        /// Returns {zoom_in_pos, zoom_out_pos}.
        std::pair<geom::Point, geom::Point> compute_zoom_button_positions(
            geom::Point const& logical_top_left,
            geom::Size const& logical_size,
            float mag) const
        {
            auto const [vis_x, vis_y, vis_w, vis_h] = compute_visual_bounds(logical_top_left, logical_size, mag);

            geom::Width const stack_width{drag_handle_diameter};

            auto const x = vis_x + geom::as_delta(vis_w) - geom::as_delta(stack_width);
            auto const y = vis_y + geom::as_delta(vis_h / 2) - geom::as_delta(zoom_stack_height / 2);

            return {
                geom::Point{x, y},
                geom::Point{x, y + geom::DeltaY{drag_handle_diameter} + geom::DeltaY{zoom_stack_padding}}};
        }

        /// Repositions all handle and button indicators for the given surface geometry.
        /// No-op when following the cursor.
        void reposition_all_handles(
            geom::Point const& tl,
            geom::Size const& sz,
            float mag)
        {
            if (follow_cursor)
                return;

            auto const [dp, rp] = compute_both_handle_positions(tl, sz, mag);
            auto const [zp, zm] = compute_zoom_button_positions(tl, sz, mag);
            drag.move_to(dp);
            resize.move_to(rp);
            zoom_in.move_to(zp);
            zoom_out.move_to(zm);
        }

        void set_magnification(float new_magnification, RenderSceneIntoSurface& render_scene_into_surface)
        {
            if (auto const surf = surface.lock())
            {
                auto const old_logical_rect = render_scene_into_surface.capture_area();

                if (!follow_cursor)
                {
                    auto const old_surface_centre =
                        center(surf->top_left(), old_logical_rect.size);

                    magnification = new_magnification;
                    auto const new_logical_size = logical_size_from_visual(visual_size, magnification);
                    auto const new_surface_top_left = old_surface_centre -
                        geom::Displacement{
                            geom::as_delta(new_logical_size.width / 2),
                            geom::as_delta(new_logical_size.height / 2)};

                    apply_positioned_geometry(
                        surf, new_surface_top_left, *this, render_scene_into_surface);
                }
                else
                {
                    auto const old_logical_centre =
                        center(old_logical_rect.top_left, old_logical_rect.size);

                    magnification = new_magnification;
                    auto const new_logical_size = logical_size_from_visual(visual_size, magnification);
                    auto const new_logical_top_left = old_logical_centre -
                        geom::Displacement{
                            geom::as_delta(new_logical_size.width / 2),
                            geom::as_delta(new_logical_size.height / 2)};

                    apply_visual_geometry(surf, new_logical_top_left, *this, render_scene_into_surface);
                }
            }
            else
            {
                magnification = new_magnification;
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

        /// Creates and registers concrete observers on each handle. Guards against
        /// missing indicators.
        void attach_observers(Self* self)
        {
            drag.attach_observer<DragHandleObserver>(self);
            resize.attach_observer<ResizeDragObserver>(self);
            zoom_in.attach_observer<ZoomButtonObserver>(self, +zoom_step);
            zoom_out.attach_observer<ZoomButtonObserver>(self, -zoom_step);
        }

        std::weak_ptr<mi::CursorObserverMultiplexer> cursor_multiplexer;
        std::shared_ptr<Observer> observer;
        std::shared_ptr<DisplayConfigObserver> display_config_observer;
        std::weak_ptr<ms::Surface> surface;
        Handle drag;
        Handle resize;
        Handle zoom_in;
        Handle zoom_out;
        std::weak_ptr<msh::SurfaceStack> handle_surface_stack;

        geom::Point cursor_pos;
        geom::Rectangle screen_bounds;
        float magnification{default_magnification};
        /// The physical (screen) size of the magnifier.  When the magnification
        /// level changes the logical capture size is recalculated from this so
        /// that the magnifier's on-screen footprint stays the same.
        geom::Size visual_size;
        bool default_enabled{false};
        bool follow_cursor{true};
    };

    struct GeometryPositions
    {
        geom::Point logical_top_left;
        geom::Point surface_top_left;
    };

    /// Applies independent logical capture and surface presentation positions.
    static void apply_geometry(
        std::shared_ptr<ms::Surface> const& surf,
        GeometryPositions const& positions,
        State& s,
        RenderSceneIntoSurface& render_scene_into_surface)
    {
        auto const logical_size = logical_size_from_visual(s.visual_size, s.magnification);
        render_scene_into_surface.capture_area(geom::Rectangle{positions.logical_top_left, logical_size});
        // capture_area() moves the backing surface to the capture position, so
        // apply the independent presentation position afterwards.
        surf->move_to(positions.surface_top_left);
        surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(s.magnification, s.magnification, 1)));
        s.reposition_all_handles(positions.surface_top_left, logical_size, s.magnification);
    }

    /// Applies normal placement, where capture and presentation positions match.
    static void apply_visual_geometry(
        std::shared_ptr<ms::Surface> const& surf,
        geom::Point const& logical_top_left,
        State& s,
        RenderSceneIntoSurface& render_scene_into_surface)
    {
        apply_geometry(
            surf,
            {
                .logical_top_left = logical_top_left,
                .surface_top_left = logical_top_left,
            },
            s,
            render_scene_into_surface);
    }

    /// Keeps the visual magnifier on-screen and maps its position proportionally
    /// onto the logical capture's available screen range.
    static void apply_positioned_geometry(
        std::shared_ptr<ms::Surface> const& surf,
        geom::Point const& desired_top_left,
        State& s,
        RenderSceneIntoSurface& render_scene_into_surface)
    {
        if (s.follow_cursor)
        {
            apply_visual_geometry(surf, desired_top_left, s, render_scene_into_surface);
            return;
        }

        auto const logical_size = logical_size_from_visual(s.visual_size, s.magnification);
        auto const surface_top_left =
            s.clamp_position_to_screen(desired_top_left, logical_size, s.magnification);
        auto const logical_top_left =
            s.capture_position_for_surface(surface_top_left, logical_size, s.magnification);
        apply_geometry(
            surf,
            {
                .logical_top_left = logical_top_left,
                .surface_top_left = surface_top_left,
            },
            s,
            render_scene_into_surface);
    }

    /// Applies visual geometry with the magnifier's logical top-left computed
    /// from its current visual size so the surface is centred on the cursor.
    static void place_at_cursor(
        std::shared_ptr<ms::Surface> const& surf,
        State& s,
        RenderSceneIntoSurface& render_scene_into_surface)
    {
        auto const logical_size = logical_size_from_visual(s.visual_size, s.magnification);
        auto top_left = surface_top_left_from_cursor(logical_size, s.cursor_pos);

        apply_positioned_geometry(surf, top_left, s, render_scene_into_surface);
    }

    class Observer : public mi::CursorObserver
    {
    public:
        explicit Observer(Self* self) : self(self) {}

        void cursor_moved_to(float abs_x, float abs_y) override
        {
            auto s = self->state.lock();
            s->cursor_pos = geom::Point{abs_x, abs_y};

            if (!s->follow_cursor)
                return;

            auto const surf = s->surface.lock();
            if (!surf)
                return;

            self->place_at_cursor(surf, *s, self->render_scene_into_surface);
        }

        void pointer_usable() override {}
        void pointer_unusable() override {}
        void image_set_to(std::shared_ptr<mg::CursorImage>) override {}

    private:
        Self* self;
    };

    /// Base for observers that turn a pointer/touch drag on a handle surface
    /// into on_drag_start()/on_drag_move() callbacks. Both callbacks are invoked
    /// with the magnifier mutex held; grab_abs holds the drag's grab position.
    class HandleObserver : public ms::NullSurfaceObserver
    {
    public:
        explicit HandleObserver(Self* self) : self(self) {}

        void input_consumed(
            ms::Surface const*,
            std::shared_ptr<MirEvent const> const& event) override
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
            else if (action == mir_pointer_action_motion && drag_active &&
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
                drag_start_magnifier_pos.x + delta.dx,
                drag_start_magnifier_pos.y + delta.dy};

            if (auto const surf = s.surface.lock())
            {
                self->apply_positioned_geometry(
                    surf, new_surface_top_left, s, self->render_scene_into_surface);
            }
        }

        geom::Point drag_start_magnifier_pos{};
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
            auto const tl = surf->top_left();
            auto const sz = self->render_scene_into_surface.capture_area().size;
            float const mag = s.magnification;

            float const outer = (mag + 1.0f) / 2.0f;
            pin_pos = tl + geom::Displacement{geom::as_delta(outer * sz.width), geom::as_delta(outer * sz.height)};
        }

        void on_drag_move(State& s, geom::Point point) override
        {
            float const mag = s.magnification;

            // Visual size from the finger position to the pinned corner
            geom::Width const raw_vis_w{geom::as_width(pin_pos.x - point.x)};
            geom::Height const raw_vis_h {geom::as_height(pin_pos.y - point.y)};

            // Enforce the surface's minimum on-screen footprint before converting
            // to a logical size (rounding up so the visual size never dips below
            // the minimum), and cap the logical size at a sane maximum.
            auto const clamped_visual_size = clamp_visual_size(geom::Size{raw_vis_w, raw_vis_h});
            auto const to_logical_dimension = [mag](auto visual_dimension)
            {
                return decltype(visual_dimension){
                    std::min(static_cast<int>(std::ceil(visual_dimension.as_value() / mag)), 1000)};
            };

            geom::Size const new_logical_size{
                to_logical_dimension(clamped_visual_size.width),
                to_logical_dimension(clamped_visual_size.height)};

            // Back-solve the logical top-left from the pinned visual corner, inverting
            // the visual-edge identity above so the pinned corner stays fixed.
            float const outer = (mag + 1.0f) / 2.0f;

            geom::Point const new_surface_top_left{
                pin_pos -
                    geom::Displacement{
                        geom::as_delta(outer * new_logical_size.width),
                        geom::as_delta(outer * new_logical_size.height),
                    },
            };

            auto const surf = s.surface.lock();
            if (!surf)
                return;

            s.visual_size = new_logical_size * mag;

            self->apply_positioned_geometry(
                surf, new_surface_top_left, s, self->render_scene_into_surface);
        }

        geom::Point pin_pos{};
    };

    /// Adjusts the magnification level when a zoom button is tapped or touched.
    class ZoomButtonObserver : public ms::NullSurfaceObserver
    {
    public:
        ZoomButtonObserver(Self* self, float delta) : self{self}, delta{delta} {}

        void input_consumed(
            ms::Surface const*,
            std::shared_ptr<MirEvent const> const& event) override
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
            s.set_magnification(
                std::clamp(s.magnification + delta, min_magnification + zoom_step, max_magnification),
                self->render_scene_into_surface);
        }

        Self* self;
        float delta;
    };

    RenderSceneIntoSurface render_scene_into_surface;
    mir::Synchronised<State> state;
    std::shared_ptr<InputFilter> input_filter;
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

auto miral::Magnifier::Self::InputFilter::handle_pointer(
    MirPointerEvent const& event,
    State const& state) -> bool
{
    auto const action = event.action();
    if (action == mir_pointer_action_button_down && !pointer_gesture_consumed)
    {
        pointer_gesture_consumed =
            event.position().transform([&state](auto const& position)
                { return state.point_is_on_magnifier_surface(position); }).value_or(false);
    }

    if (pointer_gesture_consumed)
    {
        auto const consumed = *pointer_gesture_consumed;
        if (action == mir_pointer_action_button_up && event.buttons() == 0)
            pointer_gesture_consumed.reset();
        return consumed;
    }

    return event.position().transform([&state](auto const& position)
        { return state.point_is_on_magnifier_surface(position); }).value_or(false);
}

auto miral::Magnifier::Self::InputFilter::handle_touch(
    MirTouchEvent const& event,
    State const& state) -> bool
{
    if (!touch_gesture_consumed)
    {
        bool starts_on_magnifier = false;
        for (auto i = 0u; i != event.pointer_count(); ++i)
        {
            starts_on_magnifier =
                starts_on_magnifier || state.point_is_on_magnifier_surface(event.position(i));
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

    auto s = state.lock();
    s->screen_bounds = rects.size() > 0 ? rects.bounding_rectangle() : geom::Rectangle{};
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
        {
            magnification(val.value_or(default_magnification));
        });
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
        mir::log_warning(
            "Magnification should be between %.2f and %.2f",
            min_magnification,
            max_magnification);

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
