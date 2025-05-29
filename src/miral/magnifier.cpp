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
#include "mir/events/pointer_event.h"
#include "mir/input/composite_event_filter.h"
#include "mir/scene/surface.h"

namespace mi = mir::input;
namespace ms = mir::scene;
namespace geom = mir::geometry;

class miral::Magnifier::Self
{
public:
    Self()
    {
        render_scene_into_surface.capture_area(geom::Rectangle{{300, 300}, geom::Size(400, 300)});
    }

    void operator()(mir::Server& server)
    {
        composite_event_filter = server.the_composite_event_filter();
        filter = std::make_shared<Filter>(this);
        composite_event_filter->append(filter);

        render_scene_into_surface.on_surface_ready([this](auto const& surf)
        {
            surface = surf;
        });

        render_scene_into_surface(server);
    }

private:
    class Filter : public mi::EventFilter
    {
    public:
        explicit Filter(Self* self) : self(self) {}

        bool handle(MirEvent const& event) override
        {
            if (mir_event_get_type(&event) != mir_event_type_input)
                return false;

            auto const input_event = mir_event_get_input_event(&event);
            if (mir_input_event_get_type(input_event) != mir_input_event_type_pointer)
                return false;

            auto const pointer_event = mir_input_event_get_pointer_event(input_event);
            if (mir_pointer_event_action(pointer_event) != mir_pointer_action_motion)
                return false;

            auto const window_size = self->surface->window_size();
            if (auto const position = pointer_event->position())
            {
                auto const capture_position = geom::Point{
                    position->x.as_value() - window_size.width.as_value() / 2.f,
                    position->y.as_value() - window_size.height.as_value() / 2.f
                };
                self->surface->move_to(capture_position);
                self->render_scene_into_surface.capture_area(
                    geom::Rectangle{capture_position, window_size});
            }

            return false;
        }

    private:
        Self* self;
    };

    friend Filter;

    RenderSceneIntoSurface render_scene_into_surface;
    std::shared_ptr<mi::CompositeEventFilter> composite_event_filter;
    std::shared_ptr<Filter> filter;
    std::shared_ptr<ms::Surface> surface;
};

miral::Magnifier::Magnifier()
    : self(std::make_shared<Self>())
{
}

void miral::Magnifier::operator()(mir::Server& server)
{
    self->operator()(server);
}

