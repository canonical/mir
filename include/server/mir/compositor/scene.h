/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include "mir/geometry/forward.h"

#include <memory>
#include <functional>

namespace mir
{
namespace compositor
{
class BufferStream;
class CompositingCriteria;

class FilterForScene
{
public:
    virtual ~FilterForScene() {}

    virtual bool operator()(CompositingCriteria const&) = 0;

protected:
    FilterForScene() = default;
    FilterForScene(const FilterForScene&) = delete;
    FilterForScene& operator=(const FilterForScene&) = delete;
};

class OperatorForScene
{
public:
    virtual ~OperatorForScene() {}

    virtual void operator()(CompositingCriteria const&, BufferStream&) = 0;

protected:
    OperatorForScene() = default;
    OperatorForScene(const OperatorForScene&) = delete;
    OperatorForScene& operator=(const OperatorForScene&) = delete;

};

class Scene
{
public:
    virtual ~Scene() {}

    // Back to front; normal rendering order
    virtual void for_each_if(FilterForScene& filter, OperatorForScene& op) = 0;

    // Front to back; as used when scanning for occlusions
    virtual void reverse_for_each_if(FilterForScene& filter,
                                     OperatorForScene& op) = 0;

    /**
     * Sets a callback to be called whenever the state of the
     * Scene changes.
     *
     * The supplied callback should not directly or indirectly (e.g.,
     * by changing a property of a surface) change the state of
     * the Scene, otherwise a deadlock may occur.
     */
    virtual void set_change_callback(std::function<void()> const& f) = 0;

    // Implement BasicLockable, to temporarily lock scene state:
    virtual void lock() = 0;
    virtual void unlock() = 0;

protected:
    Scene() = default;

private:
    Scene(Scene const&) = delete;
    Scene& operator=(Scene const&) = delete;
};

}
}

#endif /* MIR_COMPOSITOR_SCENE_H_ */
