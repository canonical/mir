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
#include "touchspot_image.c"

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_writer.h"
#include "mir/graphics/renderable.h"
#include "mir/geometry/dimensions.h"
#include "mir/input/scene.h"

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
geom::Size const touchspot_size = {touchspot_image.width, touchspot_image.height};
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
        return false;
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
    
// TouchspotRenderable    
    void move_center_to(geom::Point pos)
    {
        std::lock_guard<std::mutex> lg(guard);
        position = {pos.x.as_int() - touchspot_image.width/2, pos.y.as_int() - touchspot_image.height/2};
    }

    
private:
    std::shared_ptr<mg::Buffer> const buffer_;
    
    std::mutex guard;
    geom::Point position;
};


mi::TouchspotController::TouchspotController(std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mg::BufferWriter> const& buffer_writer,
    std::shared_ptr<mi::Scene> const& scene)
    : touchspot_buffer(allocator->alloc_buffer({touchspot_size, touchspot_pixel_format, mg::BufferUsage::software})),
      scene(scene),
      enabled(false),
      renderables_in_use(0)
{
    unsigned int const pixels_size = touchspot_size.width.as_uint32_t()*touchspot_size.height.as_uint32_t() *
        MIR_BYTES_PER_PIXEL(touchspot_pixel_format);
    
    buffer_writer->write(*touchspot_buffer, touchspot_image.pixel_data, pixels_size);
}

// Here we assign a set of touchspots to a set of renderables we maintain. This function requires
// and maintains the following invariant:
//    For each member of touchspot_renderables with index i (T_i), 
//    T_i is present in the scene if i < renderables_in_use, and
//    not present in the scene otherwise.
// Our assignment algorithm proceeds as follows:
//    1. If we are not enabled, ensure no renderables are in use, and return.
//    2. Otherwise, ensure we have enough prepared renderables to represent each
//       touch spot.
//    3. For each renderable we have prepared:
//       a. Is the renderable index less than the number of touches we are trying to visualize?
//          If so assign the renderable to the touch. If the index is >= renderables_in_use
//          add the renderable to the scene and update renderables in use.
//       b. Otherwise, if renderables_in_use is greater than the number of touches
//          we are trying to visualize, remove the renderable from the scene and
//          update renderables in use.
void mi::TouchspotController::visualize_touches(std::vector<Spot> const& touches)
{
    {
    std::lock_guard<std::mutex> lg(guard);
    
    unsigned int const num_touches = enabled ? touches.size() : 0;

    while (touchspot_renderables.size() < num_touches)
        touchspot_renderables.push_back(std::make_shared<TouchspotRenderable>(touchspot_buffer));
    
    for (unsigned int i = 0; i < num_touches; i++)
    {
        auto const& renderable = touchspot_renderables[i];
        
        renderable->move_center_to(touches[i].touch_location);
        if (i >= renderables_in_use)
            scene->add_input_visualization(renderable);
    }
    
    for (unsigned int i = num_touches; i < renderables_in_use; i++)
        scene->remove_input_visualization(touchspot_renderables[i]);
    
    renderables_in_use = num_touches;
    } // release mutex

    // TODO (hackish): We may have just moved renderables which with the current
    // architecture of surface observers will not trigger a propagation to the
    // compositor damage callback we need this "emit_scene_changed".
    scene->emit_scene_changed();
}

void mi::TouchspotController::enable()
{
    std::lock_guard<std::mutex> lg(guard);
    enabled = true;
}

void mi::TouchspotController::disable()
{
    std::lock_guard<std::mutex> lg(guard);
    enabled = false;
}
