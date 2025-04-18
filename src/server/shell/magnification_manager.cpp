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

#include "mir/compositor/scene_element.h"
#include "mir/compositor/screen_shooter.h"
#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "mir/frontend/surface_stack.h"
#include "mir/graphics/cursor.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/renderable.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/scene.h"
#include "mir/scene/null_observer.h"
#include "mir/scene/scene_change_notification.h"
#include "mir/renderer/sw/pixel_source.h"

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mrs = mir::renderer::software;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{
class MagnificationRenderable : public mg::Renderable
{
public:
    explicit MagnificationRenderable(std::shared_ptr<mg::Buffer> const& buffer)
        : buffer_(buffer) {}

    ID id() const override
    {
        return this;
    }

    std::shared_ptr<mg::Buffer> buffer() const override
    {
        return buffer_;
    }

    geom::Rectangle screen_position() const override
    {
        return {
            position,
            buffer_->size()
        };
    }

    geom::RectangleD src_bounds() const override
    {
        return {{0, 0}, buffer_->size()};
    }

    std::optional<geom::Rectangle> clip_area() const override
    {
        return std::optional<geom::Rectangle>();
    }

    glm::mat4 transformation() const override
    {
        return glm::mat4{
            magnification, 0.0, 0.0, 0.0,
            0.0, -magnification, 0.0, 0.0,
            0.0, 0.0, magnification, 0.0,
            0.0, 0.0, 0.0, 1.0
        };
    }

    float alpha() const override
    {
        return 1.0;
    }

    bool shaped() const override
    {
        return true;
    }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return std::nullopt;
    }

    geom::Point position;
    float magnification = 2.f;
private:
    std::shared_ptr<mg::Buffer> buffer_;
};
}

class msh::BasicMagnificationManager::Self : public mi::EventFilter
{
public:
    Self(std::shared_ptr<mi::Scene> const& scene,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<compositor::ScreenShooter> const& screen_shooter,
        std::shared_ptr<frontend::SurfaceStack> const& surface_stack,
        std::shared_ptr<graphics::Cursor> const& cursor)
        : scene_change_notification(std::make_shared<ms::SceneChangeNotification>(
            [this]{ update(); },
            [this](geom::Rectangle const& damage)
            {
                geom::Rectangle const r(
                   renderable->position,
                   renderable->buffer()->size());
                if (r.overlaps(damage))
                    update();
            }
          )),
          scene(scene),
          allocator{allocator},
          screen_shooter{screen_shooter},
          surface_stack{surface_stack},
          cursor{cursor},
          size{geom::Size(400, 300)},
          buffer(allocator->alloc_software_buffer(size, mir_pixel_format_argb_8888)),
          write_buffer(mrs::as_write_mappable_buffer(buffer)),
          renderable(std::make_shared<MagnificationRenderable>(buffer))
    {
    }

    bool handle(MirEvent const& event) override
    {
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

        renderable->position = {
            (position_opt->x.as_value() - size.width.as_int() / 2.f),
            (position_opt->y.as_value() - size.height.as_int() / 2.f)
        };
        return false;
    }

    void update()
    {
        if (!enabled)
            return;

        if (is_updating)
            return;

        is_updating = true;
        geom::Rectangle const r(
            renderable->position,
            renderable->buffer()->size());
        screen_shooter->capture_with_filter(write_buffer, r,
            [renderable=renderable, cursor=cursor](std::shared_ptr<compositor::SceneElement const> const& scene_element)
            {
                if (cursor->is(scene_element->renderable()))
                    return false;

                return scene_element->renderable() != renderable;
            },
            [&](auto const)
            {
                scene->emit_scene_changed();
                is_updating = false;
            });
    }

    std::shared_ptr<scene::SceneChangeNotification> scene_change_notification;
    std::shared_ptr<mi::Scene> const scene;
    std::shared_ptr<graphics::GraphicBufferAllocator> allocator;
    std::shared_ptr<compositor::ScreenShooter> screen_shooter;
    std::shared_ptr<frontend::SurfaceStack> surface_stack;
    std::shared_ptr<graphics::Cursor> cursor;
    geom::Size size;
    std::shared_ptr<mg::Buffer> buffer;
    std::shared_ptr<mrs::WriteMappableBuffer> write_buffer;
    std::shared_ptr<MagnificationRenderable> renderable;
    bool enabled = false;
    bool is_updating = false;
};

msh::BasicMagnificationManager::BasicMagnificationManager(
    std::shared_ptr<input::CompositeEventFilter> const& filter,
    std::shared_ptr<input::Scene> const& scene,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<compositor::ScreenShooter> const& screen_shooter,
    std::shared_ptr<frontend::SurfaceStack> const& surface_stack,
    std::shared_ptr<graphics::Cursor> const& cursor)
    : self(std::make_shared<Self>(scene, allocator, screen_shooter, surface_stack, cursor))
{
    filter->prepend(self);

    surface_stack->add_observer(self->scene_change_notification);
    enabled(true); // TODO: Do not assume enabled
}

msh::BasicMagnificationManager::~BasicMagnificationManager()
{
    self->surface_stack->remove_observer(self->scene_change_notification);
}

void mir::shell::BasicMagnificationManager::enabled(bool enabled)
{
    bool const last_enabled = self->enabled;
    self->enabled = enabled;
    if (self->enabled != last_enabled)
    {
        if (self->enabled)
        {
            self->scene->add_input_visualization(self->renderable);
            self->cursor->scale(self->renderable->magnification);
        }
        else
        {
            self->scene->remove_input_visualization(self->renderable);
            // TODO: Should we return the cursor back to a pre-magnified scale, or is 1.0 ok?
            self->cursor->scale(1.0f);
        }

        self->update();
    }
}

void mir::shell::BasicMagnificationManager::magnification(float magnification)
{
    self->renderable->magnification = magnification;
    self->update();
}
