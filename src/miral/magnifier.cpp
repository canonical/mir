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
#include "software_buffer_stream.h"
#include <miral/magnifier.h>

#include <input-method-unstable-v2_wrapper.h>
#include <wayland_wrapper.h>

#include <miral/live_config.h>
#include <mir/log.h>
#include <mir/server.h>
#include <mir/graphics/display_configuration.h>
#include <mir/graphics/display_configuration_observer.h>
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
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{
using miral::BufferPool;
using miral::create_stream_info;

auto const default_capture_width = 150;
auto const default_capture_height = 150;
auto const default_magnification = 1.5f;

/// Diameter in pixels of the circular drag handle shown at the corner of the magnifier.
auto const drag_handle_diameter = 48;

enum class HandleKind { drag, resize };

class DragHandleIndicator : public ms::BasicSurface
{
public:
    DragHandleIndicator(
        geom::Rectangle const& initial_rect,
        HandleKind kind,
        mc::CompositorID capture_id,
        std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<ms::SceneReport> const& scene_report,
        std::shared_ptr<mir::ObserverRegistrar<mg::DisplayConfigurationObserver>> const& display_config_registrar) :
        BasicSurface(
            "magnifier-drag-handle",
            initial_rect,
            mir_pointer_unconfined,
            create_stream_info(),
            nullptr,
            scene_report,
            display_config_registrar),
        pool(
            [allocator, initial_rect]
            { return allocator->alloc_software_buffer(initial_rect.size, mir_pixel_format_argb_8888); },
            2)
    {
        this->kind = kind;
        capture_compositor_id = capture_id;
        set_depth_layer(mir_depth_layer_always_on_top);
        set_focus_mode(mir_focus_mode_disabled);
        hide();
    }

    bool input_area_contains(geom::Point const& point) const override
    { return BasicSurface::input_area_contains(point); }

    mg::RenderableList generate_renderables(mc::CompositorID id) const override
    {
        if (id == capture_compositor_id)
            return {};

        auto buffer = pool.claim();
        if (kind == HandleKind::drag)
            fill_drag_buffer(*buffer);
        else
            fill_resize_buffer(*buffer, resize_corner_left, resize_corner_top);
        auto const sz = window_size();
        get_streams().begin()->stream->submit_buffer(buffer, sz, geom::RectangleD{{0, 0}, sz});
        return BasicSurface::generate_renderables(id);
    }

    void set_resize_corner(bool left, bool top)
    {
        resize_corner_left = left;
        resize_corner_top  = top;
    }

private:
    static void fill_drag_buffer(mg::Buffer& buffer)
    {
        auto const writable = mrs::as_write_mappable(std::shared_ptr<mg::Buffer>(&buffer, [](mg::Buffer*) {}));
        auto const mapping  = writable->map_writeable();
        auto* const pixels  = reinterpret_cast<uint8_t*>(mapping->data());
        int const stride    = mapping->stride().as_int();
        int const w         = buffer.size().width.as_int();
        int const h         = buffer.size().height.as_int();

        // For mir_pixel_format_argb_8888 on little-endian: memory layout [B, G, R, A].
        auto const set_pixel = [&](int x, int y, uint8_t grey, uint8_t alpha)
        {
            uint8_t* p = pixels + y * stride + x * 4;
            p[0] = grey; p[1] = grey; p[2] = grey; p[3] = alpha;
        };

        // Start fully transparent.
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                set_pixel(x, y, 0, 0);

        // Draw a filled circle with a semi-transparent dark background.
        float const cx   = (w - 1) / 2.0f;
        float const cy   = (h - 1) / 2.0f;
        float const r    = std::min(cx, cy);
        float const r_sq = r * r;

        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                float const dx = x - cx;
                float const dy = y - cy;
                if (dx * dx + dy * dy <= r_sq)
                    set_pixel(x, y, 40, 200);
            }
        }

        // Draw 6-dot grip icon (2 rows × 3 cols of 2×2 px dots) centred in the circle.
        static int constexpr dot_px      = 2;
        static int constexpr dot_spacing = 5;
        static int constexpr n_cols      = 3;
        static int constexpr n_rows      = 2;
        int const grip_w = (n_cols - 1) * dot_spacing + dot_px;
        int const grip_h = (n_rows - 1) * dot_spacing + dot_px;
        int const ox     = std::max(0, (w - grip_w) / 2);
        int const oy     = std::max(0, (h - grip_h) / 2);

        for (int row = 0; row < n_rows; ++row)
            for (int col = 0; col < n_cols; ++col)
            {
                int const ddx = ox + col * dot_spacing;
                int const ddy = oy + row * dot_spacing;
                for (int py = ddy; py < std::min(ddy + dot_px, h); ++py)
                    for (int px = ddx; px < std::min(ddx + dot_px, w); ++px)
                        set_pixel(px, py, 210, 230);
            }
    }

    static void fill_resize_buffer(mg::Buffer& buffer, bool corner_left, bool corner_top)
    {
        auto const writable = mrs::as_write_mappable(std::shared_ptr<mg::Buffer>(&buffer, [](mg::Buffer*) {}));
        auto const mapping  = writable->map_writeable();
        auto* const pixels  = reinterpret_cast<uint8_t*>(mapping->data());
        int const stride    = mapping->stride().as_int();
        int const w         = buffer.size().width.as_int();
        int const h         = buffer.size().height.as_int();

        auto const set_pixel = [&](int x, int y, uint8_t grey, uint8_t alpha)
        {
            if (x < 0 || x >= w || y < 0 || y >= h) return;
            uint8_t* p = pixels + y * stride + x * 4;
            p[0] = grey; p[1] = grey; p[2] = grey; p[3] = alpha;
        };

        // Start fully transparent.
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                set_pixel(x, y, 0, 0);

        // Draw a filled circle with a semi-transparent dark background.
        float const cx_f  = (w - 1) / 2.0f;
        float const cy_f  = (h - 1) / 2.0f;
        float const r     = std::min(cx_f, cy_f);
        float const r_sq  = r * r;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
            {
                float const dx = x - cx_f, dy = y - cy_f;
                if (dx * dx + dy * dy <= r_sq)
                    set_pixel(x, y, 40, 200);
            }

        // Corner bracket: two 2-px arms meeting at the active corner.
        static int constexpr arm_len   = 12;
        static int constexpr thickness = 2;
        int const corner_x = corner_left ? w / 4 : 3 * w / 4;
        int const corner_y = corner_top  ? h / 4 : 3 * h / 4;
        int const dx_dir   = corner_left ? 1 : -1;
        int const dy_dir   = corner_top  ? 1 : -1;

        // Horizontal arm
        for (int t = 0; t < thickness; ++t)
            for (int i = 0; i <= arm_len; ++i)
                set_pixel(corner_x + dx_dir * i, corner_y + t * dy_dir, 210, 230);

        // Vertical arm (skip corner pixel to avoid double-draw)
        for (int t = 0; t < thickness; ++t)
            for (int i = 1; i <= arm_len; ++i)
                set_pixel(corner_x + t * dx_dir, corner_y + dy_dir * i, 210, 230);
    }

    HandleKind kind{HandleKind::drag};
    bool resize_corner_left{true};
    bool resize_corner_top{true};
    BufferPool mutable pool;
    mc::CompositorID capture_compositor_id{nullptr};
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

            display_config_observer = std::make_shared<DisplayConfigObserver>(this);
            server.the_display_configuration_observer_registrar()->register_interest(display_config_observer);

            // Create the drag handle indicator.
            geom::Rectangle const handle_rect{
                {0, 0},
                {geom::Width{drag_handle_diameter}, geom::Height{drag_handle_diameter}}};

            auto const capture_id = render_scene_into_surface.capture_compositor_id();
            drag_handle_indicator = std::make_shared<DragHandleIndicator>(
                handle_rect,
                HandleKind::drag,
                capture_id,
                server.the_buffer_allocator(),
                server.the_scene_report(),
                server.the_display_configuration_observer_registrar());

            server.the_surface_stack()->add_surface(
                drag_handle_indicator, mi::InputReceptionMode::normal);
            drag_handle_indicator->set_cursor_image(server.the_default_cursor_image());
            handle_surface_stack = server.the_surface_stack();

            resize_handle_indicator = std::make_shared<DragHandleIndicator>(
                handle_rect,
                HandleKind::resize,
                capture_id,
                server.the_buffer_allocator(),
                server.the_scene_report(),
                server.the_display_configuration_observer_registrar());

            server.the_surface_stack()->add_surface(
                resize_handle_indicator, mi::InputReceptionMode::normal);
            resize_handle_indicator->set_cursor_image(server.the_default_cursor_image());
        });

        server.add_stop_callback([&]
        {
            std::lock_guard lock(mutex);
            if (auto const locked = cursor_multiplexer.lock())
                locked->unregister_interest(*observer);

            if (display_config_observer)
            {
                server.the_display_configuration_observer_registrar()->unregister_interest(*display_config_observer);
                display_config_observer.reset();
            }

            if (auto const locked = handle_surface_stack.lock())
            {
                locked->remove_surface(drag_handle_indicator);
                locked->remove_surface(resize_handle_indicator);
            }
            drag_handle_indicator.reset();
            resize_handle_indicator.reset();

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
                auto const top_left = surface_top_left_from_cursor(surf->window_size(), cursor_pos);
                surf->move_to(top_left);
                render_scene_into_surface.capture_area(geom::Rectangle{top_left, surf->window_size()});
                auto [dp, rp] = compute_both_handle_positions(top_left, surf->window_size(), magnification);
                if (drag_handle_indicator)
                {
                    drag_handle_indicator->move_to(dp);
                    drag_handle_indicator->show();
                }
                if (resize_handle_indicator)
                {
                    resize_handle_indicator->move_to(rp);
                    resize_handle_indicator->show();
                }
            }
            surf->show();
        }
        else
        {
            surf->hide();
            if (drag_handle_indicator)
                drag_handle_indicator->hide();
            if (resize_handle_indicator)
                resize_handle_indicator->hide();
        }
    }

    void set_magnification(float new_magnification)
    {
        std::lock_guard lock(mutex);
        magnification = new_magnification;
        if (auto const surf = surface.lock())
        {
            surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
            if (decoupled && (drag_handle_indicator || resize_handle_indicator))
            {
                auto [dp, rp] = compute_both_handle_positions(surf->top_left(), surf->window_size(), magnification);
                if (drag_handle_indicator)
                    drag_handle_indicator->move_to(dp);
                if (resize_handle_indicator)
                    resize_handle_indicator->move_to(rp);
            }
        }
    }

    void set_capture_size(geom::Size const& size)
    {
        std::lock_guard lock(mutex);
        auto const surf = surface.lock();
        auto const pos = surf ? surf->top_left() : surface_top_left_from_cursor(size, cursor_pos);
        render_scene_into_surface.capture_area({pos, size});
        if (decoupled && surf && (drag_handle_indicator || resize_handle_indicator))
        {
            auto [dp, rp] = compute_both_handle_positions(pos, size, magnification);
            if (drag_handle_indicator)
                drag_handle_indicator->move_to(dp);
            if (resize_handle_indicator)
                resize_handle_indicator->move_to(rp);
        }
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
                    indicator_drag_observer = std::make_shared<IndicatorDragObserver>(this);
                    drag_handle_indicator->register_interest(indicator_drag_observer);
                }
                if (resize_handle_indicator)
                {
                    resize_drag_observer = std::make_shared<ResizeDragObserver>(this);
                    resize_handle_indicator->register_interest(resize_drag_observer);
                }
                if (default_enabled && (drag_handle_indicator || resize_handle_indicator))
                {
                    auto [dp, rp] = compute_both_handle_positions(
                        surf->top_left(), surf->window_size(), magnification);
                    if (drag_handle_indicator)
                    {
                        drag_handle_indicator->move_to(dp);
                        drag_handle_indicator->show();
                    }
                    if (resize_handle_indicator)
                    {
                        resize_handle_indicator->move_to(rp);
                        resize_handle_indicator->show();
                    }
                }
            }
        }
        else
        {
            if (drag_handle_indicator)
            {
                if (indicator_drag_observer)
                {
                    drag_handle_indicator->unregister_interest(*indicator_drag_observer);
                    indicator_drag_observer.reset();
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

            auto const capture_position = surface_top_left_from_cursor(surf->window_size(), self->cursor_pos);
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

    /// Observes input on the drag handle indicator and moves both surfaces.
    class IndicatorDragObserver : public ms::NullSurfaceObserver
    {
    public:
        explicit IndicatorDragObserver(Self* self) : self(self) {}

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
            auto const ax = static_cast<int>(
                std::round(mir_pointer_event_axis_value(pev, mir_pointer_axis_x)));
            auto const ay = static_cast<int>(
                std::round(mir_pointer_event_axis_value(pev, mir_pointer_axis_y)));

            std::lock_guard lock(self->mutex);
            if (action == mir_pointer_action_button_down &&
                mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                start_drag(ax, ay);
            else if (action == mir_pointer_action_motion && drag_active &&
                     mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                apply_drag(ax, ay);
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
                start_drag(ax, ay);
            else if (action == mir_touch_action_change && drag_active)
                apply_drag(ax, ay);
            else if (action == mir_touch_action_up)
                drag_active = false;
        }

        void start_drag(int ax, int ay)
        {
            drag_active = true;
            grab_abs = {geom::DeltaX{ax}, geom::DeltaY{ay}};
            if (auto const surf = self->surface.lock())
                drag_start_magnifier_pos = surf->top_left();
        }

        void apply_drag(int ax, int ay)
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
                auto [dp, rp] = self->compute_both_handle_positions(
                    new_pos, surf->window_size(), self->magnification);
                if (self->drag_handle_indicator)
                    self->drag_handle_indicator->move_to(dp);
                if (self->resize_handle_indicator)
                    self->resize_handle_indicator->move_to(rp);
            }
        }

        Self* self;
        bool drag_active{false};
        geom::Displacement grab_abs{};
        geom::Point drag_start_magnifier_pos{};
    };

    /// Observes input on the resize handle indicator and changes the magnifier capture size.
    class ResizeDragObserver : public ms::NullSurfaceObserver
    {
    public:
        explicit ResizeDragObserver(Self* self) : self(self) {}

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
            auto const ax = static_cast<int>(
                std::round(mir_pointer_event_axis_value(pev, mir_pointer_axis_x)));
            auto const ay = static_cast<int>(
                std::round(mir_pointer_event_axis_value(pev, mir_pointer_axis_y)));

            std::lock_guard lock(self->mutex);
            if (action == mir_pointer_action_button_down &&
                mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                start_drag(ax, ay);
            else if (action == mir_pointer_action_motion && drag_active &&
                     mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                apply_drag(ax, ay);
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
                start_drag(ax, ay);
            else if (action == mir_touch_action_change && drag_active)
                apply_drag(ax, ay);
            else if (action == mir_touch_action_up)
                drag_active = false;
        }

        void start_drag(int ax, int ay)
        {
            drag_active = true;
            grab_abs = {geom::DeltaX{ax}, geom::DeltaY{ay}};

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

        void apply_drag(int ax, int ay)
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

            auto const [dp, rp] = self->compute_both_handle_positions(new_logical_tl, new_logical_size, mag);
            if (self->drag_handle_indicator)
                self->drag_handle_indicator->move_to(dp);
            if (self->resize_handle_indicator)
                self->resize_handle_indicator->move_to(rp);
        }

        Self* self;
        bool drag_active{false};
        geom::Displacement grab_abs{};
        geom::Point pin_pos{};
        bool pin_left{false};
        bool pin_above{false};
    };

    class DisplayConfigObserver : public mg::DisplayConfigurationObserver
    {
    public:
        explicit DisplayConfigObserver(Self* self) : self(self) {}

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
            std::shared_ptr<mir::scene::Session> const&,
            std::shared_ptr<mg::DisplayConfiguration> const&) override {}
        void session_configuration_removed(
            std::shared_ptr<mir::scene::Session> const&) override {}
        void configuration_failed(
            std::shared_ptr<mg::DisplayConfiguration const> const&,
            std::exception const&) override {}
        void catastrophic_configuration_error(
            std::shared_ptr<mg::DisplayConfiguration const> const&,
            std::exception const&) override {}
        void configuration_updated_for_session(
            std::shared_ptr<mir::scene::Session> const&,
            std::shared_ptr<mg::DisplayConfiguration const> const&) override {}

    private:
        void update_bounds(std::shared_ptr<mg::DisplayConfiguration const> const& config)
        {
            geom::Rectangles rects;
            config->for_each_output([&rects](mg::DisplayConfigurationOutput const& output)
            {
                if (output.used && output.connected)
                    rects.add(output.extents());
            });

            std::lock_guard lock(self->mutex);
            if (rects.size() > 0)
                self->screen_bounds = rects.bounding_rectangle();
        }

        Self* self;
    };

    static geom::Point surface_top_left_from_cursor(
        geom::Size const& window_size,
        geom::Point const& cursor_pos)
    {
        return geom::Point{
            cursor_pos.x.as_int() - window_size.width.as_int() / 2,
            cursor_pos.y.as_int() - window_size.height.as_int() / 2};
    }

    /// Computes the position of the circular drag handle indicator.
    ///
    /// The handle is placed at the visual bottom-right corner of the magnifier.
    /// Each axis flips independently when the handle would go outside the screen:
    ///   - Right edge clip  → flip to bottom-left
    ///   - Bottom edge clip → flip to top-right
    ///   - Both             → flip to top-left
    geom::Point compute_handle_position(
        geom::Point const& logical_top_left,
        geom::Size const& logical_size,
        float mag) const
    {
        int const vis_w = static_cast<int>(mag * logical_size.width.as_int());
        int const vis_h = static_cast<int>(mag * logical_size.height.as_int());

        // Visual top-left of the magnifier.
        int const vis_x = logical_top_left.x.as_int()
            - static_cast<int>((mag - 1.0f) / 2.0f * logical_size.width.as_int());
        int const vis_y = logical_top_left.y.as_int()
            - static_cast<int>((mag - 1.0f) / 2.0f * logical_size.height.as_int());

        // Default: circle centred on the bottom-right corner of the visual magnifier,
        // so it sits half outside the content area.
        int hx = vis_x + vis_w - drag_handle_diameter / 2;
        int hy = vis_y + vis_h - drag_handle_diameter / 2;

        int const screen_right  = screen_bounds.top_left.x.as_int()
            + screen_bounds.size.width.as_int();
        int const screen_bottom = screen_bounds.top_left.y.as_int()
            + screen_bounds.size.height.as_int();

        // Flip x-axis: centre on left corner instead.
        if (hx + drag_handle_diameter > screen_right)
            hx = vis_x - drag_handle_diameter / 2;

        // Flip y-axis: centre on top corner instead.
        if (hy + drag_handle_diameter > screen_bottom)
            hy = vis_y - drag_handle_diameter / 2;

        hx = std::clamp(hx, screen_bounds.top_left.x.as_int(),
                        screen_bounds.top_left.x.as_int()
                        + screen_bounds.size.width.as_int() - drag_handle_diameter);
        hy = std::clamp(hy, screen_bounds.top_left.y.as_int(),
                        screen_bounds.top_left.y.as_int()
                        + screen_bounds.size.height.as_int() - drag_handle_diameter);

        return geom::Point{hx, hy};
    }

    /// Computes the position of the circular resize handle indicator.
    ///
    /// The handle is placed at the visual top-left corner of the magnifier.
    /// Each axis flips independently when the handle would go outside the screen:
    ///   - Left edge clip → flip to top-right
    ///   - Top edge clip  → flip to bottom-left
    ///   - Both           → flip to bottom-right
    geom::Point compute_resize_handle_position(
        geom::Point const& logical_top_left,
        geom::Size const& logical_size,
        float mag) const
    {
        int const vis_w = static_cast<int>(mag * logical_size.width.as_int());
        int const vis_h = static_cast<int>(mag * logical_size.height.as_int());

        // Visual top-left of the magnifier.
        int const vis_x = logical_top_left.x.as_int()
            - static_cast<int>((mag - 1.0f) / 2.0f * logical_size.width.as_int());
        int const vis_y = logical_top_left.y.as_int()
            - static_cast<int>((mag - 1.0f) / 2.0f * logical_size.height.as_int());

        // Default: circle centred on the top-left corner of the visual magnifier,
        // so it sits half outside the content area.
        int hx = vis_x - drag_handle_diameter / 2;
        int hy = vis_y - drag_handle_diameter / 2;

        int const screen_left = screen_bounds.top_left.x.as_int();
        int const screen_top  = screen_bounds.top_left.y.as_int();

        // Flip x-axis: centre on right corner instead.
        if (hx < screen_left)
            hx = vis_x + vis_w - drag_handle_diameter / 2;

        // Flip y-axis: centre on bottom corner instead.
        if (hy < screen_top)
            hy = vis_y + vis_h - drag_handle_diameter / 2;

        hx = std::clamp(hx, screen_left,
                        screen_left + screen_bounds.size.width.as_int() - drag_handle_diameter);
        hy = std::clamp(hy, screen_top,
                        screen_top + screen_bounds.size.height.as_int() - drag_handle_diameter);

        return geom::Point{hx, hy};
    }

    /// If the two handles overlap, push the drag handle diagonally inward toward
    /// the screen centre so both remain accessible.  The resize handle keeps its position.
    void separate_if_overlapping(geom::Point& drag_pos, geom::Point& resize_pos) const
    {
        int const dx = drag_pos.x.as_int() - resize_pos.x.as_int();
        int const dy = drag_pos.y.as_int() - resize_pos.y.as_int();

        if (dx * dx + dy * dy >= drag_handle_diameter * drag_handle_diameter)
            return;

        int const screen_cx = screen_bounds.top_left.x.as_int()
            + screen_bounds.size.width.as_int() / 2;
        int const screen_cy = screen_bounds.top_left.y.as_int()
            + screen_bounds.size.height.as_int() / 2;

        auto const clamp_on_screen = [this](int x, int y)
        {
            return geom::Point{
                std::clamp(x,
                           screen_bounds.top_left.x.as_int(),
                           screen_bounds.top_left.x.as_int()
                               + screen_bounds.size.width.as_int() - drag_handle_diameter),
                std::clamp(y,
                           screen_bounds.top_left.y.as_int(),
                           screen_bounds.top_left.y.as_int()
                               + screen_bounds.size.height.as_int() - drag_handle_diameter)};
        };

        auto const overlaps = [](geom::Point const& a, geom::Point const& b)
        {
            int const ox = a.x.as_int() - b.x.as_int();
            int const oy = a.y.as_int() - b.y.as_int();
            return ox * ox + oy * oy < drag_handle_diameter * drag_handle_diameter;
        };

        // Prefer pushing the drag handle outward (away from the screen centre,
        // deeper into the corner margin) so it stays clear of the magnified
        // content. If the corner leaves no room (outward clamps back into an
        // overlap) fall back to pushing inward.
        int const out_x = (resize_pos.x.as_int() < screen_cx) ? -drag_handle_diameter : drag_handle_diameter;
        int const out_y = (resize_pos.y.as_int() < screen_cy) ? -drag_handle_diameter : drag_handle_diameter;

        auto const outward = clamp_on_screen(
            resize_pos.x.as_int() + out_x, resize_pos.y.as_int() + out_y);

        if (!overlaps(outward, resize_pos))
        {
            drag_pos = outward;
            return;
        }

        drag_pos = clamp_on_screen(
            resize_pos.x.as_int() - out_x, resize_pos.y.as_int() - out_y);
    }

    std::pair<geom::Point, geom::Point> compute_both_handle_positions(
        geom::Point const& logical_top_left,
        geom::Size const& logical_size,
        float mag) const
    {
        auto drag_pos   = compute_handle_position(logical_top_left, logical_size, mag);
        auto resize_pos = compute_resize_handle_position(logical_top_left, logical_size, mag);
        separate_if_overlapping(drag_pos, resize_pos);

        if (resize_handle_indicator)
        {
            auto const [left, top] = determine_resize_corner(resize_pos, logical_top_left, logical_size);
            resize_handle_indicator->set_resize_corner(left, top);
        }

        return {drag_pos, resize_pos};
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

    std::mutex mutex;
    RenderSceneIntoSurface render_scene_into_surface;
    std::weak_ptr<mi::CursorObserverMultiplexer> cursor_multiplexer;
    std::shared_ptr<Observer> observer;
    std::shared_ptr<DisplayConfigObserver> display_config_observer;
    std::weak_ptr<ms::Surface> surface;
    geom::Rectangle screen_bounds;
    std::shared_ptr<DragHandleIndicator> drag_handle_indicator;
    std::weak_ptr<msh::SurfaceStack> handle_surface_stack;
    std::shared_ptr<IndicatorDragObserver> indicator_drag_observer;
    std::shared_ptr<DragHandleIndicator> resize_handle_indicator;
    std::shared_ptr<ResizeDragObserver> resize_drag_observer;
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
        [this](live_config::Key const& key, std::optional<float> val)
        {
            if (val.has_value() && *val <= 1.f)
            {
                mir::log_warning(
                    "Config key '%s' should be greater than or equal to 1",
                    key.to_string().c_str());
                return;
            }

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
}

miral::Magnifier& miral::Magnifier::enable(bool enabled)
{
    self->set_enable(enabled);
    return *this;
}

miral::Magnifier& miral::Magnifier::magnification(float magnification)
{
    if (magnification <= 1.f)
    {
        mir::log_warning(
            "Magnification should be greater than or equal to 1");
        return *this;
    }

    self->set_magnification(magnification);
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
