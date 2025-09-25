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
#include "miral/magnifier.h"

#include <input-method-unstable-v2_wrapper.h>
#include <wayland_wrapper.h>

#include "miral/live_config.h"
#include "mir/log.h"
#include "mir/server.h"
#include "mir/input/cursor_observer.h"
#include "mir/input/cursor_observer_multiplexer.h"
#include "mir/scene/surface.h"

#include <glm/gtc/matrix_transform.hpp>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;

namespace
{
auto const default_capture_width = 150;
auto const default_capture_height = 150;
auto const default_magnification = 1.5f;
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
        });

        server.add_stop_callback([&]
        {
            std::lock_guard lock(mutex);
            if (auto const locked = cursor_multiplexer.lock())
                locked->unregister_interest(*observer);
        });
    }

    void set_enable(bool enable)
    {
        std::lock_guard lock(mutex);
        default_enabled = enable;
        if (auto const surf = surface.lock())
        {
            if (enable)
                surf->show();
            else
                surf->hide();
        }
    }

    void set_magnification(float new_magnification)
    {
        std::lock_guard lock(mutex);
        magnification = new_magnification;
        if (auto const surf = surface.lock())
            surf->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
    }

    void set_capture_size(geom::Size const& size)
    {
        std::lock_guard lock(mutex);
        auto const capture_position = Observer::cursor_position_to_capture_position(size, cursor_pos);
        render_scene_into_surface.capture_area({ capture_position, size });
    }

    geom::Size current_size() const
    {
        return render_scene_into_surface.capture_area().size;
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
            auto const surf = self->surface.lock();
            if (!surf)
                return;

            auto const capture_position = cursor_position_to_capture_position(
                surf->window_size(),
                self->cursor_pos);
            surf->move_to(capture_position);
            self->render_scene_into_surface.capture_area(
                geom::Rectangle{capture_position, surf->window_size()});
        }

        void pointer_usable() override {}
        void pointer_unusable() override {}

        static geom::Point cursor_position_to_capture_position(
            geom::Size const& window_size,
            geom::Point const& cursor_pos)
        {
            return geom::Point{
                cursor_pos.x.as_value() - window_size.width.as_value() / 2,
                cursor_pos.y.as_value() - window_size.height.as_value() / 2
            };
        }

    private:
        Self* self;
    };

    friend Observer;

    std::mutex mutex;
    RenderSceneIntoSurface render_scene_into_surface;
    std::weak_ptr<mi::CursorObserverMultiplexer> cursor_multiplexer;
    std::shared_ptr<Observer> observer;
    std::weak_ptr<ms::Surface> surface;
    geom::Point cursor_pos;
    float magnification = default_magnification;
    bool default_enabled = false;
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

void miral::Magnifier::operator()(mir::Server& server)
{
    self->init(server);
}
