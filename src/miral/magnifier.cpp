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

#include "magnifier_layout.h"
#include "render_scene_into_surface.h"

#include <input-method-unstable-v2_wrapper.h>
#include <wayland_wrapper.h>

#include <miral/live_config.h>
#include <mir/log.h>
#include <mir/server.h>
#include <mir/synchronised.h>
#include <mir/graphics/display_configuration.h>
#include <mir/graphics/display_configuration_observer.h>
#include <mir/geometry/rectangles.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/scene/surface.h>
#include <mir/observer_registrar.h>
#include <mir/main_loop.h>

#include <glm/gtc/matrix_transform.hpp>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mg = mir::graphics;

namespace
{
auto const default_capture_width = 300;
auto const default_capture_height = 300;
/// The lowest magnification that still reads as visibly magnified; below
/// this the magnifier is hard to distinguish from the unmagnified scene.
auto const min_magnification = 1.25f;
auto const default_magnification = 1.25f;
auto const max_magnification = 8.0f;

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

    void apply_geometry(miral::MagnifierLayout::Placement const& placement)
    {
        preferred_visual_size = placement.preferred_visual_size;
        render_scene_into_surface.capture_area(placement.capture_area);

        if (auto const surf = surface.lock())
        {
            surf->move_to(placement.untransformed_surface_top_left);
            surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
        }
    }

    std::weak_ptr<ms::Surface> surface;
    geom::Point cursor_pos;
    geom::Rectangles screen_bounds;
    float magnification{default_magnification};
    /// The on-screen size the user has asked the magnifier to be. Persistent
    /// intent: when the magnification changes the capture size is recomputed
    /// from this so the magnifier's on-screen footprint stays the same.
    geom::Size preferred_visual_size;
    bool default_enabled{false};
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
                server.the_main_loop()->spawn([=, this, &server] { this->post_init(server); });
            });

        server.add_stop_callback(
            [&]
            {
                auto const cursor_mux = cursor_multiplexer.lock();
                if (cursor_mux && cursor_observer)
                    cursor_mux->unregister_interest(*cursor_observer);

                if (display_config_observer)
                    server.the_display_configuration_observer_registrar()->unregister_interest(
                        *display_config_observer);
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

        if (auto const surf = s->surface.lock(); surf && s->default_enabled)
            place_at_cursor(*s);
    }

    void set_enable(bool enable)
    {
        auto s = state.lock();
        s->default_enabled = enable;
        if (auto const surf = s->surface.lock())
        {
            if (enable)
                surf->show();
            else
                surf->hide();
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

            s.apply_geometry(s.layout().place_following_cursor_at(old_placement.scaling_center()));
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
        s->apply_geometry(layout.place_following_cursor_at(s->cursor_pos));
    }

    geom::Size current_size() const { return state.lock()->render_scene_into_surface.capture_area().size; }

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

    /// Applies visual geometry with the magnifier's logical top-left computed
    /// from its current visual size so the surface is centred on the cursor.
    void place_at_cursor(State& s)
    {
        s.apply_geometry(s.layout().place_following_cursor_at(s.cursor_pos));
    }

    class CursorObserver : public mi::CursorObserver
    {
    public:
        explicit CursorObserver(Self* self) : self(self) {}

        void cursor_moved_to(float abs_x, float abs_y) override
        {
            auto s = self->state.lock();
            s->cursor_pos = geom::Point{abs_x, abs_y};

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
        auto const placement = s->layout().place_following_cursor_at(s->cursor_pos);
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

void miral::Magnifier::operator()(mir::Server& server)
{
    self->init(server);
}
