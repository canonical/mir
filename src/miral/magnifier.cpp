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
#include <mir/log.h>
#include <mir/server.h>
#include <mir/synchronised.h>
#include <mir/graphics/display.h>
#include <mir/geometry/rectangles.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/scene/surface.h>
#include <mir/scene/null_surface_observer.h>
#include <mir/scene/basic_surface.h>
#include <mir/graphics/graphic_buffer_allocator.h>
#include <mir/shell/surface_stack.h>
#include <mir/observer_registrar.h>

#include <glm/gtc/matrix_transform.hpp>


namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mrs = mir::renderer::software;
namespace msh = mir::shell;

namespace
{
auto const default_capture_width = 150;
auto const default_capture_height = 150;
auto const min_magnification = 1.0f;
auto const default_magnification = 1.5f;
auto const max_magnification = 8.0f;

/// Diameter in pixels of the circular drag handle shown at the corner of the magnifier.
auto const drag_handle_diameter = 48;

/// Magnification step applied by each zoom button press.
auto const zoom_step = 0.5f;

/// Painter colour/alpha constants for handle indicator graphics.
uint8_t constexpr disc_grey  = 40;
uint8_t constexpr disc_alpha = 200;
uint8_t constexpr icon_grey  = 210;
uint8_t constexpr icon_alpha = 230;

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
            stride{mapping->stride().as_int()},
            width{buffer.size().width.as_int()},
            height{buffer.size().height.as_int()}
        {
            assert(mapping->format() == format);
        }

        void clear()
        {
            std::memset(mapping->data(), 0, mapping->len());
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
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            auto* pixels = reinterpret_cast<std::uint8_t*>(mapping->data());
            auto* p = pixels + y * stride + x * MIR_BYTES_PER_PIXEL(format);
            p[0] = p[1] = p[2] = grey;
            p[3] = alpha;
        }

        static auto constexpr format = mir_pixel_format_argb_8888;
        std::shared_ptr<mrs::WriteMappable> const writable;
        std::unique_ptr<mrs::Mapping<std::byte>> const mapping;
        int const stride;
        int const width;
        int const height;
    };

    static void fill_drag_buffer(mg::Buffer& buffer)
    {
        BufferPainter painter{buffer};
        painter.clear();
        painter.fill_circle(disc_grey, disc_alpha);

        // Draw a drag-pan icon: four arrowheads (N/S/E/W) connected by slim stems.
        static int constexpr tip_dist  = 15;
        static int constexpr head_len  = 5;
        static int constexpr stem_thickness = 3;
        static int constexpr base_dist = (tip_dist - head_len);

        int const w  = painter.buffer_width();
        int const h  = painter.buffer_height();
        int const center_x = (w - 1) / 2;
        int const center_y = (h - 1) / 2;
        int const tip_n  = center_y - tip_dist;
        int const tip_s  = center_y + tip_dist;
        int const tip_w  = center_x - tip_dist;
        int const tip_e  = center_x + tip_dist;
        int const base_n = center_y - base_dist;
        int const base_s = center_y + base_dist;
        int const base_w = center_x - base_dist;
        int const base_e = center_x + base_dist;

        // Stems
        painter.draw_vertical_line(center_x - stem_thickness / 2, base_n, base_s, stem_thickness, icon_grey, icon_alpha);
        painter.draw_horizontal_line(base_w, base_e, center_y - stem_thickness / 2, stem_thickness, icon_grey, icon_alpha);

        // North arrowhead — rows from tip_n+1 to base_n, width grows by 1 px per row
        for (int y = tip_n + 1; y <= base_n; ++y)
            painter.draw_horizontal_line(center_x - (y - tip_n), center_x + (y - tip_n), y, 1, icon_grey, icon_alpha);

        // South arrowhead
        for (int y = base_s; y < tip_s; ++y)
            painter.draw_horizontal_line(center_x - (tip_s - y), center_x + (tip_s - y), y, 1, icon_grey, icon_alpha);

        // West arrowhead — columns from tip_w+1 to base_w
        for (int x = tip_w + 1; x <= base_w; ++x)
            painter.draw_vertical_line(x, center_y - (x - tip_w), center_y + (x - tip_w), 1, icon_grey, icon_alpha);

        // East arrowhead
        for (int x = base_e; x < tip_e; ++x)
            painter.draw_vertical_line(x, center_y - (tip_e - x), center_y + (tip_e - x), 1, icon_grey, icon_alpha);
    }

    static void fill_resize_buffer(mg::Buffer& buffer)
    {
        BufferPainter painter{buffer};
        painter.clear();
        painter.fill_circle(disc_grey, disc_alpha);

        // Corner bracket: two 2-px arms meeting at the top-left corner, which is
        // where the resize handle always sits within the magnifier surface.
        static int constexpr arm_len   = 12;
        static int constexpr thickness = 2;
        int const w = painter.buffer_width();
        int const h = painter.buffer_height();
        int const corner_x =  w / 4;
        int const corner_y =  h / 4;

        painter.draw_horizontal_line(corner_x, corner_x +  arm_len, corner_y, thickness, icon_grey, icon_alpha);
        painter.draw_vertical_line(corner_x, corner_y , corner_y +  arm_len, thickness, icon_grey, icon_alpha);
    }

    static void fill_zoom_buffer(mg::Buffer& buffer, bool zoom_in)
    {
        BufferPainter painter{buffer};
        painter.clear();
        painter.fill_circle(disc_grey, disc_alpha);

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
            icon_grey,
            icon_alpha);

        if (zoom_in)
            painter.draw_vertical_line(
                (w - bar_thickness) / 2,
                (h - bar_len) / 2,
                (h + bar_len) / 2 - 1,
                bar_thickness,
                icon_grey,
                icon_alpha);
    }

    miral::SoftwareBufferPool mutable pool;
};

class Handle
{
public:
    Handle() = default;

    void init(mir::Server& server, HandleKind kind)
    {
        indicator = std::make_shared<HandleIndicator>(
            handle_rect,
            kind,
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
    return geom::Point{
        cursor_pos.x.as_int() - window_size.width.as_int() / 2,
        cursor_pos.y.as_int() - window_size.height.as_int() / 2};
}

geom::Size logical_size_from_visual(geom::Size const& visual_size, float mag)
{
    return {
        geom::Width{std::max(1.0f, std::round(visual_size.width.as_value() / mag))},
        geom::Height{std::max(1.0f, std::round(visual_size.height.as_value() / mag))}};
}

/// The centre of a logical capture rectangle. The visual magnifier shares this
/// centre, so it also fixes the away direction for handle placement.
geom::Point center(geom::Point const& top_left, geom::Size const& size)
{
    return geom::Point{
        top_left.x.as_int() + size.width.as_int() / 2,
        top_left.y.as_int() + size.height.as_int() / 2};
}
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
            auto local_observer = std::make_shared<Observer>(this);

            // Keep screen bounds up to date to respond to display
            // (re)configuration, e.g. resizing a nested window.
            // Note: register_interest may call initial_configuration() synchronously,
            // which calls update_bounds(), which locks state — so we must not hold
            // the state lock across this call.
            server.the_cursor_observer_multiplexer()->register_interest(local_observer);

            auto s = state.lock();

            s->observer = local_observer;
            s->cursor_multiplexer = server.the_cursor_observer_multiplexer();
            s->visual_size = geom::Size{default_capture_width, default_capture_height}  * default_magnification;

            s->drag.init(server, HandleKind::drag);
            s->resize.init(server, HandleKind::resize);
            s->zoom_in.init(server, HandleKind::zoom_in);
            s->zoom_out.init(server, HandleKind::zoom_out);

            if (s->decoupled)
            {
                s->attach_observers(this);

                if (auto const surf = s->surface.lock(); surf && s->default_enabled)
                {
                    auto const logical_rect = render_scene_into_surface.capture_area();
                    apply_visual_geometry(surf, logical_rect.top_left, *s, render_scene_into_surface);
                    s->show_all_handles();
                }
            }
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
            Handle local_drag, local_resize, local_zoom_in, local_zoom_out;
            {
                auto s = state.lock();

                local_cursor_mux = s->cursor_multiplexer.lock();
                local_observer = s->observer;
                local_drag = std::exchange(s->drag, {});
                local_resize = std::exchange(s->resize, {});
                local_zoom_in = std::exchange(s->zoom_in, {});
                local_zoom_out = std::exchange(s->zoom_out, {});
            }

            if (local_cursor_mux && local_observer)
                local_cursor_mux->unregister_interest(*local_observer);

            for (auto* handle : {&local_drag, &local_resize, &local_zoom_in, &local_zoom_out})
                handle->reset();
        });
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
            if (s->decoupled)
            {
                // Place surface at last known cursor position when enabling in decoupled mode.
                auto const logical_size = logical_size_from_visual(s->visual_size, s->magnification);
                auto const top_left = surface_top_left_from_cursor(logical_size, s->cursor_pos);
                apply_visual_geometry(surf, top_left, *s, render_scene_into_surface);
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
        s->visual_size = size * s->magnification;
        auto const logical_top_left = surface_top_left_from_cursor(size, s->cursor_pos);

        if (auto const surf = s->surface.lock())
            apply_visual_geometry(surf, logical_top_left, *s, render_scene_into_surface);
    }

    geom::Size current_size() const
    {
        return render_scene_into_surface.capture_area().size;
    }

    void set_decoupled(bool new_decoupled)
    {
        auto s = state.lock();
        if (s->decoupled == new_decoupled)
            return;

        s->decoupled = new_decoupled;

        if (s->decoupled)
        {
            if (auto const surf = s->surface.lock())
            {
                s->attach_observers(this);
                if (s->default_enabled)
                {
                    auto const logical_rect = render_scene_into_surface.capture_area();
                    s->reposition_all_handles(logical_rect.top_left, logical_rect.size, s->magnification);
                    s->show_all_handles();
                }
            }
        }
        else
        {
            for (auto* handle : {&s->drag, &s->resize, &s->zoom_in, &s->zoom_out})
            {
                handle->detach_observer();
                handle->hide();
            }

            if (auto const surf = s->surface.lock(); surf && s->default_enabled)
            {
                auto const logical_size = logical_size_from_visual(s->visual_size, s->magnification);
                auto const top_left = surface_top_left_from_cursor(logical_size, s->cursor_pos);
                apply_visual_geometry(surf, top_left, *s, render_scene_into_surface);
            }
        }
    }

private:
    class Observer;
    class DisplayConfigObserver;

    struct State
    {
        /// Pixel-space rectangle of signed integer coordinates.
        struct VisualBounds { int x, y, w, h; };

        ///
        /// The visual rectangle is `mag` times larger than the logical one but
        /// shares the same centre, which is why the top-left is pulled back by
        /// (mag-1)/2 * size.
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
        /// No-op when not in decoupled mode.
        void reposition_all_handles(
            geom::Point const& tl,
            geom::Size const& sz,
            float mag)
        {
            if (!decoupled)
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

                // The visual magnifier is scaled about the centre of its logical
                // capture area. Keep that centre fixed so changing zoom does not
                // shift the on-screen position.
                geom::Point const old_visual_centre =
                    center(old_logical_rect.top_left, old_logical_rect.size);

                magnification = new_magnification;
                auto const new_logical_size = logical_size_from_visual(visual_size, magnification);

                geom::Point const new_logical_top_left{
                    old_visual_centre.x.as_int() - new_logical_size.width.as_int() / 2,
                    old_visual_centre.y.as_int() - new_logical_size.height.as_int() / 2};

                apply_visual_geometry(surf, new_logical_top_left, *this, render_scene_into_surface);
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
        float magnification{default_magnification};
        /// The physical (screen) size of the magnifier.  When the magnification
        /// level changes the logical capture size is recalculated from this so
        /// that the magnifier's on-screen footprint stays the same.
        geom::Size visual_size;
        bool default_enabled{false};
        bool decoupled{false};
    };

    /// Sets the logical capture area and transform so that the magnifier
    /// renders with the supplied logical top-left and keeps the same physical
    /// (visual) footprint.  The current visual size is taken from the state.
    static void apply_visual_geometry(
        std::shared_ptr<ms::Surface> const& surf,
        geom::Point const& logical_top_left,
        State& s,
        RenderSceneIntoSurface& render_scene_into_surface)
    {
        auto const logical_size = logical_size_from_visual(s.visual_size, s.magnification);
        surf->move_to(logical_top_left);
        render_scene_into_surface.capture_area(geom::Rectangle{logical_top_left, logical_size});
        surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(s.magnification, s.magnification, 1)));
        s.reposition_all_handles(logical_top_left, logical_size, s.magnification);
    }

    class Observer : public mi::CursorObserver
    {
    public:
        explicit Observer(Self* self) : self(self) {}

        void cursor_moved_to(float abs_x, float abs_y) override
        {
            auto s = self->state.lock();
            s->cursor_pos = geom::Point{abs_x, abs_y};

            // In decoupled mode the surface stays put; only update when coupled.
            if (s->decoupled)
                return;

            auto const surf = s->surface.lock();
            if (!surf)
                return;

            auto const logical_size = logical_size_from_visual(s->visual_size, s->magnification);
            auto const capture_position = surface_top_left_from_cursor(logical_size, s->cursor_pos);
            self->apply_visual_geometry(surf, capture_position, *s, self->render_scene_into_surface);
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
        virtual void on_drag_start(State& s, int ax, int ay) = 0;
        virtual void on_drag_move(State& s, int ax, int ay) = 0;

        Self* self;
        bool drag_active{false};
        geom::Displacement grab_abs{};

    private:
        void begin_drag(State& s, int ax, int ay)
        {
            drag_active = true;
            grab_abs = {geom::DeltaX{ax}, geom::DeltaY{ay}};
            on_drag_start(s, ax, ay);
        }

        void handle_pointer(MirPointerEvent const* pev)
        {
            auto const action = mir_pointer_event_action(pev);
            auto const ax = static_cast<int>(
                std::round(mir_pointer_event_axis_value(pev, mir_pointer_axis_x)));
            auto const ay = static_cast<int>(
                std::round(mir_pointer_event_axis_value(pev, mir_pointer_axis_y)));

            auto s = self->state.lock();
            if (action == mir_pointer_action_button_down &&
                mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                begin_drag(*s, ax, ay);
            else if (action == mir_pointer_action_motion && drag_active &&
                     mir_pointer_event_button_state(pev, mir_pointer_button_primary))
                on_drag_move(*s, ax, ay);
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

            auto s = self->state.lock();
            if (action == mir_touch_action_down)
                begin_drag(*s, ax, ay);
            else if (action == mir_touch_action_change && drag_active)
                on_drag_move(*s, ax, ay);
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
        void on_drag_start(State& s, int, int) override
        {
            if (auto const surf = s.surface.lock())
                drag_start_magnifier_pos = surf->top_left();
        }

        void on_drag_move(State& s, int ax, int ay) override
        {
            geom::Displacement const delta{
                geom::DeltaX{ax} - grab_abs.dx,
                geom::DeltaY{ay} - grab_abs.dy};
            geom::Point const new_logical_tl{
                drag_start_magnifier_pos.x + delta.dx,
                drag_start_magnifier_pos.y + delta.dy};

            if (auto const surf = s.surface.lock())
                self->apply_visual_geometry(surf, new_logical_tl, s, self->render_scene_into_surface);
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
    /// back-solves the logical top-left so the pinned visual corner stays put.
    class ResizeDragObserver : public HandleObserver
    {
    public:
        using HandleObserver::HandleObserver;

    protected:
        void on_drag_start(State& s, int, int) override
        {
            auto const surf = s.surface.lock();
            if (!surf)
                return;
            auto const tl = surf->top_left();
            auto const sz = self->render_scene_into_surface.capture_area().size;
            float const mag = s.magnification;

            float const outer = (mag + 1.0f) / 2.0f;
            pin_pos = geom::Point{
                tl.x.as_int() + outer * sz.width.as_int(),
                tl.y.as_int() + outer * sz.height.as_int(),
            };
        }

        void on_drag_move(State& s, int ax, int ay) override
        {
            float const mag = s.magnification;

            // Visual size from the finger position to the pinned corner
            int const raw_vis_w = pin_pos.x.as_int() - ax;
            int const raw_vis_h = pin_pos.y.as_int() - ay;

            auto const clamp = [](int v) { return std::clamp(v, 50, 1000); };
            geom::Size const new_logical_size =  geom::Size{ clamp(raw_vis_w / mag), clamp(raw_vis_h / mag) };

            // Back-solve the logical top-left from the pinned visual corner, inverting
            // the visual-edge identity above so the pinned corner stays fixed.
            float const outer = (mag + 1.0f) / 2.0f;

            int const w = new_logical_size.width.as_int();
            int const h = new_logical_size.height.as_int();

            geom::Point const new_logical_tl{
                pin_pos.x.as_int() - outer * w,
                pin_pos.y.as_int() - outer * h,
            };

            auto const surf = s.surface.lock();
            if (!surf)
                return;

            s.visual_size = new_logical_size * mag;

            self->apply_visual_geometry(surf, new_logical_tl, s, self->render_scene_into_surface);
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
                s->set_magnification(
                    std::clamp(s->magnification + delta, min_magnification + zoom_step, max_magnification),
                    self->render_scene_into_surface);
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
                s->set_magnification(
                    std::clamp(s->magnification + delta, min_magnification + zoom_step, max_magnification),
                    self->render_scene_into_surface);
            }
        }

        Self* self;
        float delta;
    };

    RenderSceneIntoSurface render_scene_into_surface;
    mir::Synchronised<State> state;
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
