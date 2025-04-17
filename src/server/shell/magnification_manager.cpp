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

#include "mir/compositor/screen_shooter.h"
#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/pointer_event.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/scene.h"
#include "mir/renderer/sw/pixel_source.h"

namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mrs = mir::renderer::software;
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
            geom::Point{0.f, 0.f},
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
        return glm::mat4(1.f);
    }

    float alpha() const override
    {
        return 1.0;
    }

    bool shaped() const override
    {
        return false;
    }

    auto surface_if_any() const -> std::optional<mir::scene::Surface const*> override
    {
        return std::nullopt;
    }

private:
    std::shared_ptr<mg::Buffer> buffer_;
};
}

class msh::BasicMagnificationManager::Self : public mi::EventFilter
{
public:
    Self(std::shared_ptr<mi::Scene> const& scene,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
        std::shared_ptr<compositor::ScreenShooter> const& screen_shooter)
        : scene(scene),
          allocator{allocator},
          screen_shooter{screen_shooter},
          buffer(allocator->alloc_software_buffer({300, 300}, mir_pixel_format_argb_8888)),
          renderable(std::make_shared<MagnificationRenderable>(buffer))
    {
        auto x = renderer::software::as_write_mappable_buffer(buffer);
        auto mapping = x->map_writeable();
        auto i = mapping->stride();
        (void)i;
        for (auto y = 0u; y < 300; y++)
        {
            for (auto x = 0u; x < 300; x++)
            {
                auto r = x * 4;
                mapping->data()[y * i.as_value() + r] = (unsigned)0xff;
                mapping->data()[y * i.as_value() + r + 1] = (unsigned)0xff;
                mapping->data()[y * i.as_value() + r + 2] = (unsigned)0x00;
                mapping->data()[y * i.as_value() + r + 3] = (unsigned)0xff;
            }
        }
    }

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
        geom::Rectangle r(
            geom::Point{300.f, 300.f},
            geom::Size{300.f, 300.f});
        auto x = renderer::software::as_write_mappable_buffer(buffer);
        screen_shooter->capture(x, r, [](auto) {});
        // auto mapping = x->map_writeable();
        // auto i = mapping->stride();
        // for (auto y = 0u; y < 300; y++)
        // {
        //     for (auto x = 0u; x < 300; x++)
        //     {
        //         auto r = x * 4;
        //         mapping->data()[y * i.as_value() + r] = (unsigned)0x00;
        //         mapping->data()[y * i.as_value() + r + 1] = (unsigned)0xff;
        //         mapping->data()[y * i.as_value() + r + 2] = (unsigned)0x00;
        //         mapping->data()[y * i.as_value() + r + 3] = (unsigned)0xff;
        //     }
        // }
    }

    std::shared_ptr<mi::Scene> const scene;
    std::shared_ptr<graphics::GraphicBufferAllocator> allocator;
    std::shared_ptr<compositor::ScreenShooter> screen_shooter;
    std::shared_ptr<mg::Buffer> buffer;
    std::shared_ptr<MagnificationRenderable> renderable;
    bool enabled = false;
    float magnification = 1.f;
    geom::PointF position;
};

msh::BasicMagnificationManager::BasicMagnificationManager(
    std::shared_ptr<input::CompositeEventFilter> const& filter,
    std::shared_ptr<input::Scene> const& scene,
    std::shared_ptr<graphics::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<compositor::ScreenShooter> const& screen_shooter)
    : self(std::make_shared<Self>(scene, allocator, screen_shooter))
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
