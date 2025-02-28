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

#include "touchspot_controller.h"
#include "touchspot_image.c"

#include "mir/geometry/displacement.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/renderable.h"
#include "mir/geometry/dimensions.h"
#include "mir/input/scene.h"
#include "mir/renderer/sw/pixel_source.h"

namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mrs = mir::renderer::software;

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
    
    geom::Rectangle screen_position() const override
    {
        return {position, buffer_->size()};
    }

    geom::RectangleD src_bounds() const override
    {
        return {{0, 0}, buffer_->size()};
    }

    std::optional<geometry::Rectangle> clip_area() const override
    {
        return std::optional<geometry::Rectangle>();
    }
    
    float alpha() const override
    {
        return 1.0;
    }
    
    glm::mat4 transformation() const override
    {
        return glm::mat4();
    }

    bool shaped() const override
    {
        return true;
    }

    auto surface_if_any() const
        -> std::optional<mir::scene::Surface const*> override
    {
        return std::nullopt;
    }

    // TouchspotRenderable
    void move_center_to(geom::Point pos)
    {
        std::lock_guard lg(guard);
        position = pos - geom::Displacement{0.5*touchspot_image.width, 0.5*touchspot_image.height};
    }

    
private:
    std::shared_ptr<mg::Buffer> const buffer_;
    
    std::mutex guard;
    geom::Point position;
};


mi::TouchspotController::TouchspotController(std::shared_ptr<mg::GraphicBufferAllocator> const& allocator,
    std::shared_ptr<mi::Scene> const& scene)
    : workaround_allocator{allocator},
      touchspot_buffer{
          mrs::alloc_buffer_with_content(
              *allocator,
              touchspot_image.pixel_data,
              touchspot_size,
              geom::Stride{touchspot_image.width * touchspot_image.bytes_per_pixel},
              touchspot_pixel_format)},
      scene(scene),
      enabled(false),
      renderables_in_use(0)
{
}

void mi::TouchspotController::visualize_touches(std::vector<Spot> const& touches)
{
    // The compositor is unable to track damage to the touchspot renderables via the SurfaceObserver
    // interface as it does with application window surfaces. So if our last action is moving a spot
    // we must ask the scene to emit a scene changed. In the case of adding or removing a visualiza-
    // tion we expect the scene to handle this for us.
    bool must_update_scene = false;

    {
    std::lock_guard lg(guard);
    
    unsigned int const num_touches = enabled ? touches.size() : 0;

    while (touchspot_renderables.size() < num_touches)
        touchspot_renderables.push_back(std::make_shared<TouchspotRenderable>(touchspot_buffer));
    
    for (unsigned int i = 0; i < num_touches; i++)
    {
        auto const& renderable = touchspot_renderables[i];
        
        renderable->move_center_to(touches[i].touch_location);
        if (i >= renderables_in_use)
            scene->add_input_visualization(renderable);
        
        must_update_scene = true;
    }
    
    for (unsigned int i = num_touches; i < renderables_in_use; i++)
        scene->remove_input_visualization(touchspot_renderables[i]);
    
    renderables_in_use = num_touches;
    } // release mutex

    // TODO (hackish): We may have just moved renderables which with the current
    // architecture of surface observers will not trigger a propagation to the
    // compositor damage callback we need this "emit_scene_changed".
    if (must_update_scene)
        scene->emit_scene_changed();
}

void mi::TouchspotController::enable()
{
    std::lock_guard lg(guard);
    enabled = true;
}

void mi::TouchspotController::disable()
{
    std::lock_guard lg(guard);
    enabled = false;
}
