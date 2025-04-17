/*
* Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "magnification_manager.h"

#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "mir/graphics/renderable.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/scene.h"

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{
class MagnificationRenderable : public mg::Renderable
{
public:
    ID id() const override
    {
        return "magnification";
    }

    std::shared_ptr<mir::graphics::Buffer> buffer() const override
    {
        return nullptr;
    }

    mir::geometry::Rectangle screen_position() const override
    {
        return {
            geom::Point{0.f, 0.f},
            geom::Size{100.f, 100.f}
        };
    }

    mir::geometry::RectangleD src_bounds() const override
    {
        return {
            geom::PointD{0.f, 0.f},
            geom::SizeD{100.f, 100.f}
        };
    }

    std::optional<mir::geometry::Rectangle> clip_area() const override
    {
        return std::nullopt;
    }

    glm::mat4 transformation() const override
    {
        return glm::mat4(1.f);
    }

    float alpha() const override
    {
        return 1.f;
    }

    bool shaped() const override
    {
        return false;
    }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return std::nullopt;
    }
};
}

class msh::BasicMagnificationManager::Self : public mi::EventFilter
{
public:
    explicit Self(std::shared_ptr<mi::Scene> const& scene)
        : scene(scene) {}

    bool handle(MirEvent const& event) override
    {
        if (!enabled)
            return false;

        auto const* input_event = event.to_input();
        if (!input_event)
            return false;

        auto const* pointer_event = input_event->to_pointer();
        if (!pointer_event)
            return false;

        if (pointer_event->action() != mir_pointer_action_motion)
            return false;

        auto const position_opt = pointer_event->position();
        if (!position_opt)
            return false;

        position = position_opt.value();
        update();
        return false;
    }

    void update()
    {
        // First, render the scene into the virtual display, which is backed by a framebuffer

        // Then,
    }

    std::shared_ptr<mi::Scene> const scene;
    std::shared_ptr<MagnificationRenderable> renderable = std::make_shared<MagnificationRenderable>();
    bool enabled = false;
    float magnification = 1.f;
    geom::PointF position;
};

msh::BasicMagnificationManager::BasicMagnificationManager(
    std::shared_ptr<input::CompositeEventFilter> const& filter,
    std::shared_ptr<input::Scene> const& scene)
    : self(std::make_shared<Self>(scene))
{
    filter->prepend(self);

    enabled(true); // TODO: Do not assume enabled
}

void mir::shell::BasicMagnificationManager::enabled(bool enabled)
{
    bool const last_enabled = self->enabled;
    self->enabled = enabled;
    if (self->enabled != last_enabled)
    {
        if (self->enabled)
            self->scene->add_input_visualization(self->renderable);
        else
            self->scene->remove_input_visualization(self->renderable);

        self->update();
    }
}

void mir::shell::BasicMagnificationManager::magnification(float magnification)
{
    self->magnification = magnification;
    self->update();
}
