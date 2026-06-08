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

#ifndef MIR_SCENE_SURFACE_SCENE_ELEMENT_H
#define MIR_SCENE_SURFACE_SCENE_ELEMENT_H

#include <mir/compositor/compositor_id.h>
#include <mir/compositor/scene_element.h>
#include <mir/graphics/renderable.h>
#include "rendering_tracker.h"

#include <memory>

namespace mir
{
namespace graphics
{
class Renderable;
}
namespace scene
{
class RenderingTracker;

class SurfaceSceneElement : public compositor::SceneElement
{
public:
    SurfaceSceneElement(
        std::string name,
        std::shared_ptr<graphics::Renderable> const& renderable,
        std::shared_ptr<RenderingTracker> const& tracker,
        compositor::CompositorID id)
        : renderable_{renderable},
          tracker{tracker},
          cid{id},
          surface_name(name)
    {
    }

    std::shared_ptr<graphics::Renderable> renderable() const override
    {
        return renderable_;
    }

    void rendered() override
    {
        tracker->rendered_in(cid);
    }

    void occluded() override
    {
        tracker->occluded_in(cid);
    }

private:
    std::shared_ptr<graphics::Renderable> const renderable_;
    std::shared_ptr<RenderingTracker> const tracker;
    compositor::CompositorID cid;
    std::string const surface_name;
};

}
}

#endif /* MIR_SCENE_SURFACE_SCENE_ELEMENT_H */
