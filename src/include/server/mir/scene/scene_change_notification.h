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

#ifndef MIR_SCENE_SCENE_CHANGE_NOTIFICATION_H_
#define MIR_SCENE_SCENE_CHANGE_NOTIFICATION_H_

#include "mir/scene/observer.h"
#include "mir/geometry/forward.h"

#include <functional>
#include <map>
#include <mutex>

namespace mir
{
namespace scene
{
class SurfaceObserver;

// A simple implementation of surface observer which forwards all changes to a provided callback.
// Also installs surface observers on each added surface which in turn forward each change to 
// said callback.
class SceneChangeNotification : public Observer
{
public:
    SceneChangeNotification(
        std::function<void()> const& scene_notify_change,
        std::function<void(mir::geometry::Rectangle const& damage)> const& damage_notify_change);

    ~SceneChangeNotification();

    void surface_added(std::shared_ptr<Surface> const& surface) override;
    void surface_removed(std::shared_ptr<Surface> const& surface) override;
    void surfaces_reordered(SurfaceSet const& affected_surfaces) override;
    
    void scene_changed() override;

    void surface_exists(std::shared_ptr<Surface> const& surface) override;
    void end_observation() override;

private:
    std::function<void()> const scene_notify_change;
    std::function<void(mir::geometry::Rectangle const& damage)> const damage_notify_change;

    std::mutex surface_observers_guard;
    std::map<Surface*, std::shared_ptr<SurfaceObserver>> surface_observers;
    
    void add_surface_observer(Surface* surface);
};

}
} // namespace mir

#endif // MIR_SCENE_SCENE_CHANGE_NOTIFICATION_H_
