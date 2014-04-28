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

#ifndef MIR_SCENE_SIMPLE_OBSERVER_H_
#define MIR_SCENE_SIMPLE_OBSERVER_H_

#include "mir/scene/observer.h"

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
class LegacySceneChangeNotification : public Observer
{
public:
    LegacySceneChangeNotification(std::function<void()> const& notify_change);
    ~LegacySceneChangeNotification();

    void surface_added(std::shared_ptr<Surface> const& surface) override;
    void surface_removed(std::shared_ptr<Surface> const& surface) override;
    void surfaces_reordered() override;

private:
    std::function<void()> const notify_change;
    
    std::mutex surface_observers_guard;
    std::map<std::weak_ptr<Surface>, std::shared_ptr<SurfaceObserver>,
        std::owner_less<std::weak_ptr<Surface>>> surface_observers;
};

}
} // namespace mir

#endif // MIR_SCENE_SIMPLE_OBSERVER_H_
