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

#include <miral/live_config.h>
#include <mir/log.h>
#include <mir/server.h>
#include <mir/synchronised.h>
#include <mir/graphics/display_configuration.h>
#include <mir/graphics/display_configuration_observer.h>
#include <mir/graphics/null_display_configuration_observer.h>
#include <mir/geometry/rectangles.h>
#include <mir/input/cursor_observer.h>
#include <mir/input/cursor_observer_multiplexer.h>
#include <mir/scene/surface.h>
#include <mir/observer_registrar.h>
#include <mir/main_loop.h>

#include <algorithm>
#include <optional>
#include <glm/gtc/matrix_transform.hpp>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mml = miral::magnifier_layout;

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
    void apply_geometry(geom::Rectangle const& new_capture_area, geom::Point const& new_surface_top_left)
    {
        render_scene_into_surface.capture_area(new_capture_area);

        if (auto const surf = surface.lock())
        {
            surf->move_to(new_surface_top_left);
            surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
        }
    }

    void apply_geometry(mml::Placement const& new_placement)
    {
        apply_geometry(new_placement.capture_area, new_placement.capture_area.top_left);
    }

    void apply_geometry(mml::FreePlacement const& new_placement)
    {
        applied_placement = new_placement;
        freely_positioned_center = geom::PointD{
            new_placement.surface_top_left +
            geom::generic::as_displacement(new_placement.capture_area.size / 2.0)};

        apply_geometry(new_placement.capture_area, new_placement.surface_top_left);
    }

    auto has_outputs() const -> bool { return screen_bounds.size() != 0; }

    std::weak_ptr<ms::Surface> surface;
    geom::Point cursor_pos;
    geom::PointD freely_positioned_center;
    geom::Rectangles screen_bounds;
    float magnification{default_magnification};
    geom::SizeD requested_visual_size{
        default_capture_width * static_cast<double>(default_magnification),
        default_capture_height * static_cast<double>(default_magnification)};
    std::optional<mml::FreePlacement> applied_placement;
    bool enabled{false};
    bool follow_cursor{true};
    miral::RenderSceneIntoSurface render_scene_into_surface;
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
        render_scene_into_surface.on_surface_ready(
            [this](auto const& surf)
            {
                auto s = state.lock();
                surf->set_transformation(
                    glm::scale(glm::mat4(1.0), glm::vec3(s->magnification, s->magnification, 1)));
                surf->set_depth_layer(mir_depth_layer_always_on_top);
                surf->set_focus_mode(mir_focus_mode_disabled);

                if (s->enabled)
                    surf->show();
                else
                    surf->hide();

                s->surface = surf;
            });

        render_scene_into_surface(server);

        server.add_init_callback(
            [&]
            {
                server.the_main_loop()->spawn([=, this, &server] { this->post_init(server); });
            });

        server.add_stop_callback(
            [&]
            {
                if (cursor_observer)
                    server.the_cursor_observer_multiplexer()->unregister_interest(*cursor_observer);

                if (display_config_observer)
                    server.the_display_configuration_observer_registrar()->unregister_interest(
                        *display_config_observer);
            });
    }

    void post_init(mir::Server& server)
    {
        cursor_observer = std::make_shared<CursorObserver>(this);
        server.the_cursor_observer_multiplexer()->register_interest(cursor_observer);

        display_config_observer = std::make_shared<DisplayConfigObserver>(*this);
        server.the_display_configuration_observer_registrar()->register_interest(display_config_observer);

        auto s = state.lock();

        if (auto const surf = s->surface.lock(); surf && s->enabled)
        {
            if (!s->follow_cursor)
            {
                s->freely_positioned_center = geom::PointD{
                    surf->top_left() +
                    geom::generic::as_displacement(s->render_scene_into_surface.capture_area().size / 2.0)};
                place_freely(*s);
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
                place_freely(*s);
            surf->show();
        }
        else
        {
            surf->hide();
        }
    }

    void set_magnification(float new_magnification)
    {
        auto const s = state.lock();
        s->magnification = new_magnification;
        s->applied_placement.reset();
        if (!s->surface.lock())
            return;

        if (s->follow_cursor)
            place_at_cursor(*s);
        else
            place_freely(*s);
    }

    void set_capture_size(geom::Size const& size)
    {
        auto s = state.lock();

        s->requested_visual_size = geom::SizeD{size} * s->magnification;
        s->applied_placement.reset();
        auto const capture_top_left = s->render_scene_into_surface.capture_area().top_left;
        s->render_scene_into_surface.capture_area({capture_top_left, size});

        if (!s->surface.lock())
            return;

        if (s->follow_cursor)
            place_at_cursor(*s);
        else
            place_freely(*s);
    }

    geom::Size current_size() const { return render_scene_into_surface.capture_area().size; }

    void follow_cursor()
    {
        auto s = state.lock();
        if (s->follow_cursor)
            return;

        s->follow_cursor = true;
        place_at_cursor(*s);
    }

    void stop_following_cursor()
    {
        auto s = state.lock();
        if (!s->follow_cursor)
            return;

        s->follow_cursor = false;

        if (auto const surf = s->surface.lock(); surf && s->enabled)
        {
            s->freely_positioned_center = geom::PointD{
                surf->top_left() +
                geom::generic::as_displacement(s->render_scene_into_surface.capture_area().size / 2.0)};
            place_freely(*s);
        }
    }

private:
    class DisplayConfigObserver : public mg::NullDisplayConfigurationObserver
    {
    public:
        DisplayConfigObserver(Self& self) : self{self} {}

        void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
        { update_bounds(config); }

        void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
        { update_bounds(config); }

    private:
        void update_bounds(std::shared_ptr<mg::DisplayConfiguration const> const& config);
        Self& self;
    };

    void apply_geometry(
        mml::Placement const& new_placement,
        miral::RenderSceneIntoSurface& render_scene_into_surface,
        State& state)
    {
        render_scene_into_surface.capture_area(new_placement.capture_area);

        if (auto const surf = state.surface.lock())
        {
            surf->move_to(new_placement.capture_area.top_left);
            surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(state.magnification, state.magnification, 1)));
        }
    }

    /// Applies visual geometry with the magnifier's logical top-left computed
    /// from its current visual size so the surface is centred on the cursor.
    void place_at_cursor(State& s)
    {
        if (!s.has_outputs())
        {
            s.applied_placement.reset();
            return;
        }

        auto const new_placement = mml::place_following_cursor(
            geom::PointD{s.cursor_pos}, s.requested_visual_size, s.screen_bounds, s.magnification);

        apply_geometry(
            new_placement,
            render_scene_into_surface,
            s);
    }

    void place_freely(State& s)
    {
        if (!s.has_outputs())
        {
            s.applied_placement.reset();
            return;
        }

        s.apply_geometry(
            mml::place_freely(
                s.freely_positioned_center,
                s.requested_visual_size,
                s.screen_bounds,
                s.magnification));
    }

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

    mir::Synchronised<State> state;
    miral::RenderSceneIntoSurface render_scene_into_surface;
    std::shared_ptr<CursorObserver> cursor_observer;
    std::shared_ptr<DisplayConfigObserver> display_config_observer;
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
    if (!s->has_outputs())
    {
        s->applied_placement.reset();
        return;
    }

    if (s->surface.lock())
    {
        if (s->follow_cursor)
            self.place_at_cursor(*s);
        else
            self.place_freely(*s);
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
