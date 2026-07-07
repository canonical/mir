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

#include "render_scene_into_surface.h"
#include "software_buffer_pool.h"
#include <miral/magnifier.h>

#include <input-method-unstable-v2_wrapper.h>
#include <wayland_wrapper.h>

#include <miral/live_config.h>
#include <mir/log.h>
#include <mir/server.h>
#include <mir/graphics/display.h>
#include <mir/geometry/rectangles.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/scene/surface.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/scene/basic_surface.h>
#include <mir/scene/scene_report.h>
#include <mir/compositor/stream.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/renderer/sw/pixel_source.h>
#include <mir/shell/surface_stack.h>
#include <mir/observer_registrar.h>

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <algorithm>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace msh = mir::shell;

namespace
{
using miral::SoftwareBufferPool;
using miral::create_stream_info;

auto const default_capture_width = 150;
auto const default_capture_height = 150;
auto const min_magnification = 1.0f;
auto const default_magnification = 1.5f;
auto const max_magnification = 8.0f;

/// Diameter in pixels of the circular drag handle shown at the corner of the magnifier.
auto const drag_handle_diameter = 48;

/// Magnification step applied by each zoom button press.
auto const zoom_step = 0.5f;

enum class HandleKind { drag, resize, zoom_in, zoom_out };

class HandleIndicator : public ms::BasicSurface
{
public:
    HandleIndicator(
        geom::Rectangle const& initial_rect,
        HandleKind kind,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<ms::SceneReport> const& scene_report,
        std::shared_ptr<mir::ObserverRegistrar<mg::DisplayConfigurationObserver>> const& display_config_registrar) :
        BasicSurface{
            "magnifier-drag-handle",
            initial_rect,
            mir_pointer_unconfined,
            create_stream_info(),
            nullptr,
            scene_report,
            display_config_registrar},
        pool{
            [allocator, initial_rect]
            { return allocator->alloc_software_buffer(initial_rect.size, mir_pixel_format_argb_8888); },
            1}
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

private:
    class BufferPainter
    {
    public:
        explicit BufferPainter(mg::Buffer& buffer) :
            writable{mrs::as_write_mappable(std::shared_ptr<mg::Buffer>(&buffer, [](mg::Buffer*) {}))},
            mapping{writable->map_writeable()},
            pixels{reinterpret_cast<uint8_t*>(mapping->data())},
            stride{mapping->stride().as_int()},
            width{buffer.size().width.as_int()},
            height{buffer.size().height.as_int()}
        {
        }

        void clear()
        {
            for (int y = 0; y < height; ++y)
                for (int x = 0; x < width; ++x)
                set_pixel(x, y, 0, 0);
        }

        void fill_circle(uint8_t grey, uint8_t alpha)
        {
            float const cx = (width - 1) / 2.0f;
            float const cy = (height - 1) / 2.0f;
            float const r = std::min(cx, cy);
            float const r_sq = r * r;

            for (int y = 0; y < height; ++y)
            {
                for (int x = 0; x < width; ++x)
                {
                    float const dx = x - cx;
                    float const dy = y - cy;
                    if (dx * dx + dy * dy <= r_sq)
                        set_pixel(x, y, grey, alpha);
                }
            }
        }

        void draw_horizontal_line(
            int x0,
            int x1,
            int y,
            int thickness,
            uint8_t grey,
            uint8_t alpha)
        {
            for (int t = 0; t < thickness; ++t)
                for (int x = std::min(x0, x1); x <= std::max(x0, x1); ++x)
                    set_pixel(x, y + t, grey, alpha);
        }

        void draw_vertical_line(
            int x,
            int y0,
            int y1,
            int thickness,
            uint8_t grey,
            uint8_t alpha)
        {
            for (int t = 0; t < thickness; ++t)
                for (int y = std::min(y0, y1); y <= std::max(y0, y1); ++y)
                    set_pixel(x + t, y, grey, alpha);
        }

        auto buffer_width() const -> int
        {
            return width;
        }

        auto buffer_height() const -> int
        {
            return height;
        }

    private:
        void set_pixel(int x, int y, uint8_t grey, uint8_t alpha)
        {
            if (x < 0 || x >= width || y < 0 || y >= height)
                return;

            // For mir_pixel_format_argb_8888 on little-endian: memory layout [B, G, R, A].
            uint8_t* p = pixels + y * stride + x * 4;
            p[0] = grey; p[1] = grey; p[2] = grey; p[3] = alpha;
        }

        std::shared_ptr<mrs::WriteMappable> const writable;
        std::unique_ptr<mrs::Mapping<std::byte>> const mapping;
        uint8_t* const pixels;
        int const stride;
        int const width;
        int const height;
    };

    static void fill_drag_buffer(mg::Buffer& buffer)
    {
        BufferPainter painter{buffer};
        painter.clear();
        painter.fill_circle(40, 200);

        // Draw a drag-pan icon: four arrowheads (N/S/E/W) connected by slim stems.
        static int constexpr tip_dist  = 15;
        static int constexpr head_len  = 5;
        static int constexpr stem_half = 1;

        int const w  = painter.buffer_width();
        int const h  = painter.buffer_height();
        int const ci = (w - 1) / 2;
        int const cj = (h - 1) / 2;
        int const tip_n  = cj - tip_dist;
        int const tip_s  = cj + tip_dist;
        int const tip_w  = ci - tip_dist;
        int const tip_e  = ci + tip_dist;
        int const base_n = cj - (tip_dist - head_len);
        int const base_s = cj + (tip_dist - head_len);
        int const base_w = ci - (tip_dist - head_len);
        int const base_e = ci + (tip_dist - head_len);

        // Stems
        painter.draw_vertical_line(ci - stem_half, base_n, base_s, stem_half * 2 + 1, 210, 230);
        painter.draw_horizontal_line(base_w, base_e, cj - stem_half, stem_half * 2 + 1, 210, 230);

        // North arrowhead — rows from tip_n+1 to base_n, width grows by 1 px per row
        for (int y = tip_n + 1; y <= base_n; ++y)
            painter.draw_horizontal_line(ci - (y - tip_n), ci + (y - tip_n), y, 1, 210, 230);

        // South arrowhead
        for (int y = base_s; y < tip_s; ++y)
            painter.draw_horizontal_line(ci - (tip_s - y), ci + (tip_s - y), y, 1, 210, 230);

        // West arrowhead — columns from tip_w+1 to base_w
        for (int x = tip_w + 1; x <= base_w; ++x)
            painter.draw_vertical_line(x, cj - (x - tip_w), cj + (x - tip_w), 1, 210, 230);

        // East arrowhead
        for (int x = base_e; x < tip_e; ++x)
            painter.draw_vertical_line(x, cj - (tip_e - x), cj + (tip_e - x), 1, 210, 230);
    }

    static void fill_resize_buffer(mg::Buffer& buffer)
    {
        BufferPainter painter{buffer};
        painter.clear();
        painter.fill_circle(40, 200);

        // Corner bracket: two 2-px arms meeting at the top-left corner, which is
        // where the resize handle always sits within the magnifier surface.
        static int constexpr arm_len   = 12;
        static int constexpr thickness = 2;
        int const w = painter.buffer_width();
        int const h = painter.buffer_height();
        int const corner_x =  w / 4;
        int const corner_y =  h / 4;

        painter.draw_horizontal_line(corner_x, corner_x + arm_len, corner_y, thickness, 210, 230);
        painter.draw_vertical_line(corner_x, corner_y, corner_y + arm_len, thickness, 210, 230);
    }

    static void fill_zoom_buffer(mg::Buffer& buffer, bool zoom_in)
    {
        BufferPainter painter{buffer};
        painter.clear();
        painter.fill_circle(40, 200);

        // Draw + (zoom in) or – (zoom out) symbol.
        int const w = painter.buffer_width();
        int const h = painter.buffer_height();
        int const bar_thickness = std::max(2, w / 12);
        int const bar_len       = w * 5 / 8;

        painter.draw_horizontal_line(
            (w - bar_len) / 2,
            (w + bar_len) / 2 - 1,
            (h - bar_thickness) / 2,
            bar_thickness,
            210,
            230);

        if (zoom_in)
            painter.draw_vertical_line(
                (w - bar_thickness) / 2,
                (h - bar_len) / 2,
                (h + bar_len) / 2 - 1,
                bar_thickness,
                210,
                230);
    }

    SoftwareBufferPool mutable pool;
};
}

class miral::Magnifier::Self
{
public:
    Self()
    {
        render_scene_into_surface
            .capture_area(geom::Rectangle{{300, 300}, geom::Size(default_capture_width, default_capture_height)})
            .overlay_cursor(false);
    }

    /// Creates a handle indicator surface, adds it to the surface stack and
    /// gives it the default cursor image.
    auto create_handle_indicator(
        mir::Server& server,
        HandleKind kind,
        geom::Rectangle const& rect) -> std::shared_ptr<HandleIndicator>
    {
        auto indicator = std::make_shared<HandleIndicator>(
            rect,
            kind,
            server.the_buffer_allocator(),
            server.the_scene_report(),
            server.the_display_configuration_observer_registrar());

        server.the_surface_stack()->add_surface(indicator, mi::InputReceptionMode::normal);
        indicator->set_cursor_image(server.the_default_cursor_image());
        return indicator;
    }

    void init(mir::Server& server)
    {
        render_scene_into_surface.on_surface_ready([this](auto const& surf)
        {
            surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
            surf->set_depth_layer(mir_depth_layer_always_on_top);
            surf->set_focus_mode(mir_focus_mode_disabled);

            if (default_enabled)
                surf->show();
            else
                surf->hide();

            std::lock_guard lock(mutex);
            surface = surf;
        });

        render_scene_into_surface(server);

        server.add_init_callback([&]
        {
            observer = std::make_shared<Observer>(this);
            server.the_cursor_observer_multiplexer()->register_interest(observer);
            cursor_multiplexer = server.the_cursor_observer_multiplexer();

            // Create the drag handle indicator.
            geom::Rectangle const handle_rect{
                {0, 0},
                {geom::Width{drag_handle_diameter}, geom::Height{drag_handle_diameter}}};

            drag_handle_indicator   = create_handle_indicator(server, HandleKind::drag, handle_rect);
            resize_handle_indicator = create_handle_indicator(server, HandleKind::resize, handle_rect);
            zoom_in_indicator       = create_handle_indicator(server, HandleKind::zoom_in, handle_rect);
            zoom_out_indicator      = create_handle_indicator(server, HandleKind::zoom_out, handle_rect);
            handle_surface_stack    = server.the_surface_stack();

            if (decoupled)
            {
                drag_handle_observer = std::make_shared<DragHandleObserver>(this);
                drag_handle_indicator->register_interest(drag_handle_observer);

                resize_drag_observer = std::make_shared<ResizeDragObserver>(this);
                resize_handle_indicator->register_interest(resize_drag_observer);

                zoom_in_observer = std::make_shared<ZoomButtonObserver>(this, +zoom_step);
                zoom_in_indicator->register_interest(zoom_in_observer);

                zoom_out_observer = std::make_shared<ZoomButtonObserver>(this, -zoom_step);
                zoom_out_indicator->register_interest(zoom_out_observer);

                if (auto const surf = surface.lock(); surf && default_enabled)
                {
                    reposition_all_handles_locked(surf->top_left(), surf->window_size(), magnification);
                    show_all_handles_locked();
                }
            }
        });

        server.add_stop_callback([&]
        {
            std::lock_guard lock(mutex);
            if (auto const locked = cursor_multiplexer.lock())
                locked->unregister_interest(*observer);

            if (drag_handle_observer)
            {
                drag_handle_indicator->unregister_interest(*drag_handle_observer);
                drag_handle_observer.reset();
            }


            if (resize_drag_observer)
            {
                resize_handle_indicator->unregister_interest(*resize_drag_observer);
                resize_drag_observer.reset();
            }

            if (zoom_in_observer)
            {
                zoom_in_indicator->unregister_interest(*zoom_in_observer);
                zoom_in_observer.reset();
            }

            if (zoom_out_observer)
            {
                zoom_out_indicator->unregister_interest(*zoom_out_observer);
                zoom_out_observer.reset();
            }

            if (auto const locked = handle_surface_stack.lock())
            {
                locked->remove_surface(drag_handle_indicator);
                locked->remove_surface(resize_handle_indicator);
                locked->remove_surface(zoom_in_indicator);
                locked->remove_surface(zoom_out_indicator);
            }

            drag_handle_indicator.reset();
            resize_handle_indicator.reset();
            zoom_in_indicator.reset();
            zoom_out_indicator.reset();
        });
    }

    void set_enable(bool enable)
    {
        std::lock_guard lock(mutex);
        default_enabled = enable;
        auto const surf = surface.lock();
        if (!surf)
            return;

        if (enable)
        {
            if (decoupled)
            {
                // Place surface at last known cursor position when enabling in decoupled mode.
                auto const top_left = surface_top_left_from_cursor(surf->window_size(), cursor_pos);
                surf->move_to(top_left);
                render_scene_into_surface.capture_area(geom::Rectangle{top_left, surf->window_size()});
                reposition_all_handles_locked(top_left, surf->window_size(), magnification);
                show_all_handles_locked();
            }
            surf->show();
        }
        else
        {
            surf->hide();
            hide_all_handles_locked();
        }
    }

    void set_magnification(float new_magnification)
    {
        std::lock_guard lock(mutex);
        set_magnification_locked(new_magnification);
    }

    void set_capture_size(geom::Size const& size)
    {
        std::lock_guard lock(mutex);
        auto const capture_position = surface_top_left_from_cursor(size, cursor_pos);
        render_scene_into_surface.capture_area({ capture_position, size });
        reposition_all_handles_locked(capture_position, size, magnification);
    }

    geom::Size current_size() const
    {
        return render_scene_into_surface.capture_area().size;
    }

    void set_decoupled(bool new_decoupled)
    {
        std::lock_guard lock(mutex);
        if (decoupled == new_decoupled)
            return;

        decoupled = new_decoupled;

        if (new_decoupled)
        {
            if (auto const surf = surface.lock())
            {
                if (drag_handle_indicator)
                {
                    drag_handle_observer = std::make_shared<DragHandleObserver>(this);
                    drag_handle_indicator->register_interest(drag_handle_observer);
                }
                if (resize_handle_indicator)
                {
                    resize_drag_observer = std::make_shared<ResizeDragObserver>(this);
                    resize_handle_indicator->register_interest(resize_drag_observer);
                }
                if (zoom_in_indicator)
                {
                    zoom_in_observer = std::make_shared<ZoomButtonObserver>(this, +zoom_step);
                    zoom_in_indicator->register_interest(zoom_in_observer);
                }
                if (zoom_out_indicator)
                {
                    zoom_out_observer = std::make_shared<ZoomButtonObserver>(this, -zoom_step);
                    zoom_out_indicator->register_interest(zoom_out_observer);
                }
                if (default_enabled)
                {
                    reposition_all_handles_locked(surf->top_left(), surf->window_size(), magnification);
                    show_all_handles_locked();
                }
            }
        }
        else
        {
            if (drag_handle_indicator)
            {
                if (drag_handle_observer)
                {
                    drag_handle_indicator->unregister_interest(*drag_handle_observer);
                    drag_handle_observer.reset();
                }
                drag_handle_indicator->hide();
            }
            if (resize_handle_indicator)
            {
                if (resize_drag_observer)
                {
                    resize_handle_indicator->unregister_interest(*resize_drag_observer);
                    resize_drag_observer.reset();
                }
                resize_handle_indicator->hide();
            }
            if (zoom_in_indicator)
            {
                if (zoom_in_observer)
                {
                    zoom_in_indicator->unregister_interest(*zoom_in_observer);
                    zoom_in_observer.reset();
                }
                zoom_in_indicator->hide();
            }
            if (zoom_out_indicator)
            {
                if (zoom_out_observer)
                {
                    zoom_out_indicator->unregister_interest(*zoom_out_observer);
                    zoom_out_observer.reset();
                }
                zoom_out_indicator->hide();
            }

            if (auto const surf = surface.lock(); surf && default_enabled)
            {
                auto const top_left = surface_top_left_from_cursor(surf->window_size(), cursor_pos);
                surf->move_to(top_left);
                render_scene_into_surface.capture_area(geom::Rectangle{top_left, surf->window_size()});
            }
        }
    }

private:
    class Observer : public mi::CursorObserver
    {
    public:
        explicit Observer(Self* self) : self(self) {}

        void cursor_moved_to(float abs_x, float abs_y) override
        {
            std::lock_guard lock(self->mutex);
            self->cursor_pos = geom::Point{abs_x, abs_y};

            // In decoupled mode the surface stays put; only update when coupled.
            if (self->decoupled)
                return;

            auto const surf = self->surface.lock();
            if (!surf)
                return;

            auto const capture_position = surface_top_left_from_cursor(
                surf->window_size(),
                self->cursor_pos);
            surf->move_to(capture_position);
            self->render_scene_into_surface.capture_area(
                geom::Rectangle{capture_position, surf->window_size()});
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
        virtual void on_drag_start(int ax, int ay) = 0;
        virtual void on_drag_move(int ax, int ay) = 0;

        Self* self;
        bool drag_active{false};
        geom::Displacement grab_abs{};

    private:
        void begin_drag(int ax, int ay)
        {
            drag_active = true;
            grab_abs = {geom::DeltaX{ax}, geom::DeltaY{ay}};
            on_drag_start(ax, ay);
        }

        void handle_pointer(MirPointerEvent const* pev)
        {
            auto const action = mir_pointer_event_action(pev);
            auto const ax = static_cast<int>(
                std::round(mir_pointer_event_axis_value(pev, mir_pointer_axis_x)));
            auto const ay = static_cast<int>(
                std::round(mir_pointer_event_axis_value(pev, mir_pointer_axis_y)));

            std::lock_guard lock(self->mutex);
            if (action == mir_pointer_action_button_down &&
                mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                begin_drag(ax, ay);
            else if (action == mir_pointer_action_motion && drag_active &&
                     mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                on_drag_move(ax, ay);
            else if (action == mir_pointer_action_button_up)
                drag_active = false;
        }

        void handle_touch(MirTouchEvent const* tev)
        {
            if (mir_touch_event_point_count(tev) != 1)
                return;
            auto const action = mir_touch_event_action(tev, 0);
            auto const ax = static_cast<int>(
                std::round(mir_touch_event_axis_value(tev, 0, mir_touch_axis_x)));
            auto const ay = static_cast<int>(
                std::round(mir_touch_event_axis_value(tev, 0, mir_touch_axis_y)));

            std::lock_guard lock(self->mutex);
            if (action == mir_touch_action_down)
                begin_drag(ax, ay);
            else if (action == mir_touch_action_change && drag_active)
                on_drag_move(ax, ay);
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
        void on_drag_start(int, int) override
        {
            if (auto const surf = self->surface.lock())
                drag_start_magnifier_pos = surf->top_left();
        }

        void on_drag_move(int ax, int ay) override
        {
            geom::Displacement const delta{
                geom::DeltaX{ax} - grab_abs.dx,
                geom::DeltaY{ay} - grab_abs.dy};
            geom::Point const new_pos{
                drag_start_magnifier_pos.x + delta.dx,
                drag_start_magnifier_pos.y + delta.dy};

            if (auto const surf = self->surface.lock())
            {
                surf->move_to(new_pos);
                self->render_scene_into_surface.capture_area(
                    geom::Rectangle{new_pos, surf->window_size()});
                self->reposition_all_handles_locked(new_pos, surf->window_size(), self->magnification);
            }
        }

        geom::Point drag_start_magnifier_pos{};
    };

    /// Observes input on the resize handle indicator and changes the magnifier capture size.
    /// Resizes the magnifier capture area when its resize handle is dragged.
    class ResizeDragObserver : public HandleObserver
    {
    public:
        using HandleObserver::HandleObserver;

    protected:
        void on_drag_start(int, int) override
        {
            auto const surf = self->surface.lock();
            if (!surf) return;
            auto const tl   = surf->top_left();
            auto const sz   = self->render_scene_into_surface.capture_area().size;
            float const mag = self->magnification;

            // Determine which corner the handle is currently at
            auto const resize_pos        = self->compute_resize_handle_position(tl, sz, mag);
            auto const [left, top]       = self->determine_resize_corner(resize_pos, tl, sz);

            // The pinned corner is diagonally opposite the active handle corner
            pin_left  = !left;
            pin_above = !top;

            float const inner = (mag - 1.0f) / 2.0f;
            float const outer = (mag + 1.0f) / 2.0f;
            pin_pos = geom::Point{
                pin_left
                    ? static_cast<int>(tl.x.as_int() - inner * sz.width.as_int())
                    : static_cast<int>(tl.x.as_int() + outer * sz.width.as_int()),
                pin_above
                    ? static_cast<int>(tl.y.as_int() - inner * sz.height.as_int())
                    : static_cast<int>(tl.y.as_int() + outer * sz.height.as_int())};
        }

        void on_drag_move(int ax, int ay) override
        {
            float const mag = self->magnification;

            // Visual size from the finger position to the pinned corner
            int const raw_vis_w = pin_left
                ? (ax - pin_pos.x.as_int())
                : (pin_pos.x.as_int() - ax);
            int const raw_vis_h = pin_above
                ? (ay - pin_pos.y.as_int())
                : (pin_pos.y.as_int() - ay);

            auto const clamp = [](int v) { return std::clamp(v, 50, 1000); };
            geom::Size const new_logical_size{
                geom::Width{ clamp(static_cast<int>(std::round(raw_vis_w / mag)))},
                geom::Height{clamp(static_cast<int>(std::round(raw_vis_h / mag)))}};

            float const inner = (mag - 1.0f) / 2.0f;
            float const outer = (mag + 1.0f) / 2.0f;
            int const w = new_logical_size.width.as_int();
            int const h = new_logical_size.height.as_int();

            geom::Point const new_logical_tl{
                pin_left
                    ? static_cast<int>(pin_pos.x.as_int() + inner * w)
                    : static_cast<int>(pin_pos.x.as_int() - outer * w),
                pin_above
                    ? static_cast<int>(pin_pos.y.as_int() + inner * h)
                    : static_cast<int>(pin_pos.y.as_int() - outer * h)};

            auto const surf = self->surface.lock();
            if (!surf) return;

            surf->move_to(new_logical_tl);
            self->render_scene_into_surface.capture_area({new_logical_tl, new_logical_size});
            self->reposition_all_handles_locked(new_logical_tl, new_logical_size, mag);
        }

        geom::Point pin_pos{};
        bool pin_left{false};
        bool pin_above{false};
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

            std::lock_guard lock(self->mutex);
            if (action == mir_pointer_action_button_down &&
                mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                self->set_magnification_locked(
                    std::clamp(self->magnification + delta, min_magnification + zoom_step, max_magnification));
        }

        void handle_touch(MirTouchEvent const* tev)
        {
            if (mir_touch_event_point_count(tev) != 1)
                return;
            auto const action = mir_touch_event_action(tev, 0);

            std::lock_guard lock(self->mutex);
            if (action == mir_touch_action_down)
                self->set_magnification_locked(
                    std::clamp(self->magnification + delta, min_magnification + zoom_step, max_magnification));
        }

        Self* self;
        float delta;
    };

    static geom::Point surface_top_left_from_cursor(
        geom::Size const& window_size,
        geom::Point const& cursor_pos)
    {
        return geom::Point{
            cursor_pos.x.as_int() - window_size.width.as_int() / 2,
            cursor_pos.y.as_int() - window_size.height.as_int() / 2};
    }

    /// Pixel-space bounds of the magnified visual surface.
    struct VisualBounds { int x, y, w, h; };

    /// Returns the pixel-space top-left and size of the visual magnifier surface
    /// given its logical capture rectangle and magnification factor.
    VisualBounds compute_visual_bounds(
        geom::Point const& logical_top_left,
        geom::Size const& logical_size,
        float mag) const
    {
        int const w = mag * logical_size.width.as_int();
        int const h = mag * logical_size.height.as_int();
        int const x = logical_top_left.x.as_int() - (mag - 1.0f) / 2.0f * logical_size.width.as_int();
        int const y = logical_top_left.y.as_int() - (mag - 1.0f) / 2.0f * logical_size.height.as_int();
        return {x, y, w, h};
    }

    /// Computes the position of the circular drag handle indicator.
    ///
    /// The handle is placed at the visual bottom-right corner of the magnifier.
    geom::Point compute_drag_handle_position(
        geom::Point const& logical_top_left,
        geom::Size const& logical_size,
        float mag) const
    {
        auto const [vis_x, vis_y, vis_w, vis_h] = compute_visual_bounds(logical_top_left, logical_size, mag);

        int hx = vis_x + vis_w - drag_handle_diameter;
        int hy = vis_y + vis_h - drag_handle_diameter;

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
        int hx = vis_x;
        int hy = vis_y;

        return geom::Point{hx, hy};
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

    /// Returns which corner the resize handle is at as (corner_left, corner_top).
    std::pair<bool, bool> determine_resize_corner(
        geom::Point const& handle_pos,
        geom::Point const& logical_top_left,
        geom::Size const& logical_size) const
    {
        int const vis_cx = logical_top_left.x.as_int() + logical_size.width.as_int()  / 2;
        int const vis_cy = logical_top_left.y.as_int() + logical_size.height.as_int() / 2;

        int const hx = handle_pos.x.as_int() + drag_handle_diameter / 2;
        int const hy = handle_pos.y.as_int() + drag_handle_diameter / 2;

        return {hx < vis_cx, hy < vis_cy};
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

        int const vertical_padding = 8;
        int const stack_width = drag_handle_diameter;
        int const stack_height = 2 * drag_handle_diameter + vertical_padding;

        int const x = vis_x + vis_w - stack_width;
        int const y = vis_y + vis_h / 2 - stack_height / 2;

        return {geom::Point{x, y}, geom::Point{x, y + drag_handle_diameter + vertical_padding}};
    }

    /// Repositions all handle and button indicators for the given capture geometry.
    /// No-op when not in decoupled mode.  Must be called with `mutex` held.
    void reposition_all_handles_locked(
        geom::Point const& tl,
        geom::Size const& sz,
        float mag)
    {
        if (!decoupled)
            return;

        auto const [dp, rp] = compute_both_handle_positions(tl, sz, mag);
        if (drag_handle_indicator)   drag_handle_indicator->move_to(dp);
        if (resize_handle_indicator) resize_handle_indicator->move_to(rp);

        auto const [zp, zm] = compute_zoom_button_positions(tl, sz, mag);
        if (zoom_in_indicator)  zoom_in_indicator->move_to(zp);
        if (zoom_out_indicator) zoom_out_indicator->move_to(zm);
    }

    /// Updates the magnification without acquiring the mutex (caller must hold it).
    void set_magnification_locked(float new_magnification)
    {
        magnification = new_magnification;
        if (auto const surf = surface.lock())
        {
            surf->set_transformation(
                glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
            reposition_all_handles_locked(surf->top_left(), surf->window_size(), magnification);
        }
    }

    void show_all_handles_locked()
    {
        if (drag_handle_indicator)   drag_handle_indicator->show();
        if (resize_handle_indicator) resize_handle_indicator->show();
        if (zoom_in_indicator)  zoom_in_indicator->show();
        if (zoom_out_indicator) zoom_out_indicator->show();
    }

    void hide_all_handles_locked()
    {
        if (drag_handle_indicator)   drag_handle_indicator->hide();
        if (resize_handle_indicator) resize_handle_indicator->hide();
        if (zoom_in_indicator)  zoom_in_indicator->hide();
        if (zoom_out_indicator) zoom_out_indicator->hide();
    }

    std::mutex mutex;
    RenderSceneIntoSurface render_scene_into_surface;
    std::weak_ptr<mi::CursorObserverMultiplexer> cursor_multiplexer;
    std::shared_ptr<Observer> observer;
    std::weak_ptr<ms::Surface> surface;
    std::shared_ptr<HandleIndicator> drag_handle_indicator;
    std::weak_ptr<msh::SurfaceStack> handle_surface_stack;
    std::shared_ptr<DragHandleObserver> drag_handle_observer;
    std::shared_ptr<HandleIndicator> resize_handle_indicator;
    std::shared_ptr<ResizeDragObserver> resize_drag_observer;
    std::shared_ptr<HandleIndicator> zoom_in_indicator;
    std::shared_ptr<HandleIndicator> zoom_out_indicator;
    std::shared_ptr<ZoomButtonObserver> zoom_in_observer;
    std::shared_ptr<ZoomButtonObserver> zoom_out_observer;
    geom::Point cursor_pos;
    float magnification{default_magnification};
    bool default_enabled{false};
    bool decoupled{false};
};

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
    config_store.add_bool_attribute(
        {"magnifier", "decouple"},
        "Whether the magnifier is decoupled from the cursor position",
        [this](live_config::Key const&, std::optional<bool> val)
        {
            if (val.has_value())
                decouple(*val);
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

miral::Magnifier& miral::Magnifier::decouple(bool decoupled)
{
    self->set_decoupled(decoupled);
    return *this;
}

void miral::Magnifier::operator()(mir::Server& server)
{
    self->init(server);
}
