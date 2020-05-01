/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_SCENE_OBSERVER_H_
#define MIR_SCENE_OBSERVER_H_

#include <memory>
#include <set>

namespace mir
{
namespace scene
{
class Surface;
using SurfaceSet = std::set<std::weak_ptr<Surface>, std::owner_less<std::weak_ptr<Surface>>>;

/// An observer for top level notifications of scene changes. In order
/// to receive more granular change notifications a user may install
/// mir::scene::SurfaceObserver in surface_added() and surface_exists().
class Observer
{
public:
    /// A new surface has been added to the scene
    /// Not called for existing surfaces, see surface_exists()
    virtual void surface_added(std::shared_ptr<Surface> const& surface) = 0;

    /// The surface has been removed from the scene
    virtual void surface_removed(std::shared_ptr<Surface> const& surface) = 0;

    /// The stacking order of surfaces has changed
    /// Affected surfaces may or may not have changed order relative to each other, and they may be at any position
    /// Non-affected surfaces will not have changed order relative to each other
    virtual void surfaces_reordered(SurfaceSet const& affected_surfaces) = 0;

    /// Used to indicate the scene has changed in some way beyond the present surfaces
    /// and will require full recomposition.
    virtual void scene_changed() = 0;

    /// Called at observer registration to notify of already existing surfaces.
    virtual void surface_exists(std::shared_ptr<Surface> const& surface) = 0;

    /// Called when observer is unregistered, for example, to provide a place to
    /// unregister SurfaceObservers which may have been added in surface_added/exists
    virtual void end_observation() = 0;

protected:
    Observer() = default;
    virtual ~Observer() = default;
    Observer(Observer const&) = delete;
    Observer& operator=(Observer const&) = delete;
};

}
} // namespace mir

#endif // MIR_SCENE_OBSERVER_H_
