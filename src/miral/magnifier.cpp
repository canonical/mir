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
#include "mir/server.h"
#include "mir/input/cursor_observer.h"
#include "mir/input/cursor_observer_multiplexer.h"
#include "mir/scene/surface.h"

#include <glm/gtc/matrix_transform.hpp>

namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;

class miral::Magnifier::Self
{
public:
    Self()
    {
        render_scene_into_surface
            .capture_area(geom::Rectangle{{300, 300}, geom::Size(400, 300)})
            .overlay_cursor(false);
    }

    ~Self()
    {
        if (observer && cursor_multiplexer)
            cursor_multiplexer->unregister_interest(*observer);
    }

    void operator()(mir::Server& server)
    {
        server.add_init_callback([&]
        {
            cursor_multiplexer = server.the_cursor_observer_multiplexer();
            observer = std::make_shared<Observer>(this);
            cursor_multiplexer->register_interest(observer);
        });

        server.add_stop_callback([&]
        {
            surface = nullptr;
        });

        render_scene_into_surface.on_surface_ready([this](auto const& surf)
        {
            std::lock_guard lock(mutex);
            surface = surf;
            surface->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
            surface->set_depth_layer(mir_depth_layer_always_on_top);
            surface->set_focus_mode(mir_focus_mode_disabled);

            if (default_enabled)
                surface->show();
            else
                surface->hide();
        });

        render_scene_into_surface(server);
    }

    void set_enable(bool enable)
    {
        std::lock_guard lock(mutex);
        default_enabled = enable;
        if (surface)
        {
            if (enable)
                surface->show();
            else
                surface->hide();
        }
    }

    void set_magnification(float new_magnification)
    {
        std::lock_guard lock(mutex);
        magnification = new_magnification;
        if (surface)
            surface->set_transformation(glm::scale(glm::mat4(1.0), glm::vec3(magnification, magnification, 1)));
    }

    void set_capture_size(geom::Size const& size)
    {
        std::lock_guard lock(mutex);
        auto const capture_position = Observer::cursor_position_to_capture_position(size, cursor_pos);
        render_scene_into_surface.capture_area({ capture_position, size });
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
            if (!self->surface)
                return;

            auto const capture_position = cursor_position_to_capture_position(
                self->surface->window_size(),
                self->cursor_pos);
            self->surface->move_to(capture_position);
            self->render_scene_into_surface.capture_area(
                geom::Rectangle{capture_position, self->surface->window_size()});
        }

        void pointer_usable() override {}
        void pointer_unusable() override {}

        static geom::Point cursor_position_to_capture_position(
            geom::Size const& window_size,
            geom::Point const& cursor_pos)
        {
            return geom::Point{
                cursor_pos.x.as_value() - window_size.width.as_value() / 2.f,
                cursor_pos.y.as_value() - window_size.height.as_value() / 2.f
            };;
        }

    private:
        Self* self;
    };

    friend Observer;

    std::mutex mutex;
    RenderSceneIntoSurface render_scene_into_surface;
    std::shared_ptr<mi::CursorObserverMultiplexer> cursor_multiplexer;
    std::shared_ptr<Observer> observer;
    std::shared_ptr<ms::Surface> surface;
    geom::Point cursor_pos;
    float magnification = 2.f;
    bool default_enabled = false;
};

miral::Magnifier::Magnifier()
    : self(std::make_shared<Self>())
{
}

miral::Magnifier& miral::Magnifier::enable(bool enabled)
{
    self->set_enable(enabled);
    return *this;
}

miral::Magnifier& miral::Magnifier::magnification(float magnification)
{
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
    self->operator()(server);
}

