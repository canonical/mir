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
#include "mir/graphics/buffer_writer.h"
#include "mir/graphics/renderable.h"
#include "mir/input/scene.h"

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
geom::Size const touchspot_size = {64, 64};
MirPixelFormat const touchspot_pixel_format = mir_pixel_format_argb_8888;
}

class mi::TouchspotRenderable : public mg::Renderable
{
public:
    TouchspotRenderable(std::shared_ptr<mg::Buffer> const& buffer)
        : buffer_(buffer),
          position({0, 0})
    {
    }

// mg::Renderable
    mg::Renderable::ID id() const override
    {
        return this;
    }
    
    std::shared_ptr<mg::Buffer> buffer() const override
    {
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
        return 1;
    }
    
    bool should_be_decorated() const
    {
        return false;
    }

// TouchspotRenderable    
    void move_to(geom::Point pos)
    {
        std::lock_guard<std::mutex> lg(guard);
        position = pos;
    }

    
private:
    std::shared_ptr<mg::Buffer> const buffer_;
    
    std::mutex guard;
    geom::Point position;
};


#include <string.h> // TODO: Remove
mi::TouchspotController::TouchspotController(std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mg::BufferWriter> const& buffer_writer,
    std::shared_ptr<mi::Scene> const& scene)
    : touchspot_buffer(allocator->alloc_buffer({touchspot_size, touchspot_pixel_format, mg::BufferUsage::software})),
      scene(scene),
      renderables_in_use(0)
{
    unsigned int const pixels_size = touchspot_size.width.as_uint32_t()*touchspot_size.height.as_uint32_t() *
        MIR_BYTES_PER_PIXEL(touchspot_pixel_format);
    
    uint32_t *pixels = new uint32_t[pixels_size];
    memset(pixels, 0xffffffff, pixels_size);
    buffer_writer->write(touchspot_buffer, pixels, pixels_size);
    delete[] pixels;
}

void mi::TouchspotController::visualize_touches(std::vector<Spot> const& touches)
{
    std::lock_guard<std::mutex> lg(guard);

    // We can assume the maximum number of touches will not grow unreasonably large
    // and so we just grow a pool of TouchspotRenderables as needed
    while (touchspot_renderables.size() < touches.size())
        touchspot_renderables.push_back(std::make_shared<TouchspotRenderable>(touchspot_buffer));

    // We act on each touchspot renderable, as it may need positioning, to be added to the scene, or removed
    // entirely.
    for (unsigned int i = 0; i < touchspot_renderables.size(); i++)
    {
        auto const& renderable = touchspot_renderables[i];
        
        // We map the touches to the first available renderables.
        if (i < touches.size())
        {
            renderable->move_to(touches[i].touch_location);

            if (renderables_in_use <= i)
            {
                renderables_in_use++;
                scene->add_overlay(renderable);
            }
        }
        // If we are using too many touch-spot renderables, we need to remove some from
        // the scene.
        else if (renderables_in_use > touches.size())
        {
            renderables_in_use--;
            scene->remove_overlay(renderable);
        }
    }
    
    // TODO (hackish): We may have just moved renderables which  will not propgate 
    // through with damage notifications out of compositor in current architecture so 
    // we need this "emit_scene_changed".
    scene->emit_scene_changed();
}
