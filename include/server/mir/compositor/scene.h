/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_COMPOSITOR_SCENE_H_
#define MIR_COMPOSITOR_SCENE_H_

#include "compositor_id.h"

#include <memory>
#include <vector>

namespace mir
{
namespace scene
{
class Observer;
}
namespace geometry
{
class Rectangle;
}

namespace compositor
{

class SceneElement;
using SceneElementSequence = std::vector<std::shared_ptr<SceneElement>>;

class Scene
{
public:
    virtual ~Scene() {}

    /**
     * Generate a valid sequence of scene elements based on the current state of the Scene.
     * \param [in] id      An arbitrary unique identifier used to distinguish
     *                     separate compositors which need to receive a sequence
     *                     for rendering. Calling with the same id will return
     *                     a new (different) sequence to that user each time. For
     *                     consistency, all callers need to determine their id
     *                     in the same way (e.g. always use "this" pointer).
     * \returns a sequence of SceneElements for the compositor id. The
     *          sequence is in stacking order from back to front.
     */
    virtual SceneElementSequence scene_elements_for(CompositorID id) = 0;

    /**
     * Return the number of additional frames that you need to render to get
     * fully up to date with the latest data in the scene. For a generic
     * "scene change" this will be just 1. For surfaces that have multiple
     * frames queued up however, it could be greater than 1. When the result
     * reaches zero, you know you have consumed all the latest data from the
     * scene.
     */
    virtual int frames_pending(CompositorID id) const = 0;

    virtual void register_compositor(CompositorID id) = 0;
    virtual void unregister_compositor(CompositorID id) = 0;

    virtual void add_observer(std::shared_ptr<scene::Observer> const& observer) = 0;
    virtual void remove_observer(std::weak_ptr<scene::Observer> const& observer) = 0;

protected:
    Scene() = default;

private:
    Scene(Scene const&) = delete;
    Scene& operator=(Scene const&) = delete;
};

}
}

#endif /* MIR_COMPOSITOR_SCENE_H_ */
