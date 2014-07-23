/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "touchspot_controller.h"

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/renderable.h"
#include "mir/input/input_targets.h"

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
geom::Size const touchspot_size = {32, 32};
MirPixelFormat const touchspot_pixel_format = mir_pixel_format_argb_8888;
}

// TODO: Need a strategy for change notifications
class mi::TouchspotRenderable : public mg::Renderable
{
public:
    TouchspotRenderable(std::shared_ptr<mg::Buffer> const& buffer)
        : buffer_(buffer),
          position({0, 0})
    {
    }

    mg::Renderable::ID id() const override
    {
        // TODO: Make sure this is ok
        return this;
    }
    
    std::shared_ptr<mg::Buffer> buffer() const override
    {
        // TODO: Locking strategy
        return buffer_;
    }
    
    bool alpha_enabled() const override
    {
        return true;
    }

    geom::Rectangle screen_position() const
    {
        return {position, buffer_->size()};
    }
    
    float alpha() const
    {
        return 1.0;
    }
    
    glm::mat4 transformation() const
    {
        return glm::mat4();
    }

    bool visible() const
    {
        return true;
    }
    
    bool shaped() const
    {
        return true;
    }
    
    int buffers_ready_for_compositor() const
    {
        // ?
        return 1;
    }
    
    void move_to(geom::Point pos)
    {
        // TODO: Notifications
        position = pos;
    }

    
private:
    std::shared_ptr<mg::Buffer> const buffer_;
    
    // TODO: Locking
    geom::Point position;
};


// TODO: Locking
#include <string.h> // TODO: Remove
mi::TouchspotController::TouchspotController(std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mi::InputTargets> const& scene)
    : touchspot_pixels(allocator->alloc_buffer({touchspot_size, touchspot_pixel_format, mg::BufferUsage::software})),
      scene(scene)
{
    unsigned int const pixels_size = touchspot_size.width.as_uint32_t()*touchspot_size.height.as_uint32_t() *
        MIR_BYTES_PER_PIXEL(touchspot_pixel_format);
    
    uint32_t *pixels = new uint32_t[pixels_size];
    memset(pixels, 0, pixels_size);
    touchspot_pixels->write(pixels, pixels_size);
    delete[] pixels;
}

void mi::TouchspotController::visualize_touches(std::vector<Spot> const& touches)
{
    while (touchspot_renderables.size() < touches.size())
        touchspot_renderables.push_back(std::make_shared<TouchspotRenderable>(touchspot_pixels));
    for (unsigned int i = 0; i < touchspot_renderables.size(); i++)
    {
        auto const& renderable = touchspot_renderables[i];
        if (i < touches.size())
        {
            renderable->move_to(touches[i].touch_location);
            if (renderable.use_count() == 1)
                scene->add_overlay(renderable);
        }
        else if (renderable.use_count() != 1)
        {
            scene->remove_overlay(renderable);
        }
    }
//    scene->emit_scene_changed();
}
