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
#include <mir/graphics/display_configuration.h>
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
auto const default_magnification = 1.5f;

/// Diameter in pixels of the circular drag handle shown at the corner of the magnifier.
auto const drag_handle_diameter = 48;

class DragHandleIndicator : public ms::BasicSurface
{
public:
    DragHandleIndicator(
        geom::Rectangle const& initial_rect,
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
        fill_grip_buffer(*buffer);
        auto const sz = window_size();
        get_streams().begin()->stream->submit_buffer(buffer, sz, geom::RectangleD{{0, 0}, sz});

        set_depth_layer(mir_depth_layer_always_on_top);
        set_focus_mode(mir_focus_mode_disabled);
        hide();
    }

private:
    static void fill_grip_buffer(mg::Buffer& buffer)
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

        // Draw a drag-pan icon: four arrowheads (N/S/E/W) connected by slim stems.
        // head_half == head_len so the half-width grows by exactly 1 px per row, with
        // no isolated single-pixel tip rows (the tip row itself is skipped).
        static int constexpr tip_dist  = 15; // distance from centre to arrow tip
        static int constexpr head_len  = 5;  // rows from stem junction to tip (== head_half)
        static int constexpr stem_half = 1;  // half-width of connecting stems (3 px total)

        int const ci     = static_cast<int>(cx);
        int const cj     = static_cast<int>(cy);
        int const tip_n  = cj - tip_dist;
        int const tip_s  = cj + tip_dist;
        int const tip_w  = ci - tip_dist;
        int const tip_e  = ci + tip_dist;
        int const base_n = cj - (tip_dist - head_len);
        int const base_s = cj + (tip_dist - head_len);
        int const base_w = ci - (tip_dist - head_len);
        int const base_e = ci + (tip_dist - head_len);

        auto const in_icon = [&](int x, int y) -> bool
        {
            // Vertical stem
            if (std::abs(x - ci) <= stem_half && y >= base_n && y <= base_s)
                return true;
            // Horizontal stem
            if (std::abs(y - cj) <= stem_half && x >= base_w && x <= base_e)
                return true;
            // North arrowhead — skip y==tip_n so the tip starts at 3 px wide, not 1
            if (y > tip_n && y <= base_n)
            {
                if (std::abs(x - ci) <= y - tip_n) return true;
            }
            // South arrowhead
            if (y >= base_s && y < tip_s)
            {
                if (std::abs(x - ci) <= tip_s - y) return true;
            }
            // West arrowhead
            if (x > tip_w && x <= base_w)
            {
                if (std::abs(y - cj) <= x - tip_w) return true;
            }
            // East arrowhead
            if (x >= base_e && x < tip_e)
            {
                if (std::abs(y - cj) <= tip_e - x) return true;
            }
            return false;
        };

        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x)
                if (in_icon(x, y))
                    set_pixel(x, y, 210, 230);
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

            drag_handle_indicator = std::make_shared<DragHandleIndicator>(
                handle_rect,
                server.the_buffer_allocator(),
                server.the_scene_report(),
                server.the_display_configuration_observer_registrar());

            server.the_surface_stack()->add_surface(
                drag_handle_indicator, mi::InputReceptionMode::normal);
            drag_handle_indicator->set_cursor_image(server.the_default_cursor_image());
            drag_handle_surface_stack = server.the_surface_stack();

            if (decoupled)
            {
                drag_handle_observer = std::make_shared<DragHandleObserver>(this);
                drag_handle_indicator->register_interest(drag_handle_observer);

                if (auto const surf = surface.lock(); surf && default_enabled)
                {
                    auto const dp = compute_drag_handle_position(surf->top_left(), surf->window_size(), magnification);
                    drag_handle_indicator->move_to(dp);
                    drag_handle_indicator->show();
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

            if (auto const locked = drag_handle_surface_stack.lock())
                locked->remove_surface(drag_handle_indicator);

            drag_handle_indicator.reset();

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
                if (drag_handle_indicator)
                {
                    drag_handle_indicator->move_to(
                        compute_drag_handle_position(top_left, surf->window_size(), magnification));
                    drag_handle_indicator->show();
                }
            }
            surf->show();
        }
        else
        {
            surf->hide();
            if (drag_handle_indicator)
                drag_handle_indicator->hide();
        }
    }

    void set_magnification(float new_magnification)
    {
        std::lock_guard lock(mutex);
        magnification = new_magnification;
        if (auto const surf = surface.lock())
        {
            surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
            if (decoupled && drag_handle_indicator)
            {
                drag_handle_indicator->move_to(
                    compute_drag_handle_position(surf->top_left(), surf->window_size(), magnification));
            }
        }
    }

    void set_capture_size(geom::Size const& size)
    {
        std::lock_guard lock(mutex);
        auto const capture_position = surface_top_left_from_cursor(size, cursor_pos);
        render_scene_into_surface.capture_area({ capture_position, size });
        if (decoupled && drag_handle_indicator)
        {
            drag_handle_indicator->move_to(compute_drag_handle_position(capture_position, size, magnification));
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
            if (auto const surf = surface.lock(); surf && drag_handle_indicator)
            {
                drag_handle_observer = std::make_shared<DragHandleObserver>(this);
                drag_handle_indicator->register_interest(drag_handle_observer);

                // Only show the handle if the magnifier is currently active.
                if (default_enabled)
                {
                    auto const dp = compute_drag_handle_position(surf->top_left(), surf->window_size(), magnification);
                    drag_handle_indicator->move_to(dp);
                    drag_handle_indicator->show();
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

    /// Observes input on the drag handle indicator and moves both surfaces.
    class DragHandleObserver : public ms::NullSurfaceObserver
    {
    public:
        explicit DragHandleObserver(Self* self) : self(self) {}

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
                if (self->drag_handle_indicator)
                {
                    self->drag_handle_indicator->move_to(
                        self->compute_drag_handle_position(
                            new_pos, surf->window_size(), self->magnification));
                }
            }
        }

        Self* self;
        bool drag_active{false};
        geom::Displacement grab_abs{};
        geom::Point drag_start_magnifier_pos{};
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
    geom::Point compute_drag_handle_position(
        geom::Point const& logical_top_left,
        geom::Size const& logical_size,
        float mag) const
    {
        int const vis_w = mag * logical_size.width.as_int();
        int const vis_h = mag * logical_size.height.as_int();

        // Visual top-left of the magnifier.
        int const vis_x = logical_top_left.x.as_int() - (mag - 1.0f) / 2.0f * logical_size.width.as_int();
        int const vis_y = logical_top_left.y.as_int() - (mag - 1.0f) / 2.0f * logical_size.height.as_int();

        int hx = vis_x + vis_w - drag_handle_diameter;
        int hy = vis_y + vis_h - drag_handle_diameter;

        return geom::Point{hx, hy};
    }

    std::mutex mutex;
    RenderSceneIntoSurface render_scene_into_surface;
    std::weak_ptr<mi::CursorObserverMultiplexer> cursor_multiplexer;
    std::shared_ptr<Observer> observer;
    std::weak_ptr<ms::Surface> surface;
    std::shared_ptr<DragHandleIndicator> drag_handle_indicator;
    std::weak_ptr<msh::SurfaceStack> drag_handle_surface_stack;
    std::shared_ptr<DragHandleObserver> drag_handle_observer;
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
