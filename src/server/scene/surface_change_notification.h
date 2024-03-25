/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_SCENE_SURFACE_CHANGE_NOTIFICATION_H_
#define MIR_SCENE_SURFACE_CHANGE_NOTIFICATION_H_

#include "mir/scene/null_surface_observer.h"

#include <functional>
#include <mutex>

namespace mir
{
namespace scene
{
class SurfaceChangeNotification : public mir::scene::NullSurfaceObserver
{
public:
    SurfaceChangeNotification(
        scene::Surface* surface,
        std::function<void()> const& notify_scene_change,
        std::function<void(geometry::Rectangle const&)> const& notify_buffer_change);

    void content_resized_to(Surface const* surf, geometry::Size const&) override;
    void moved_to(Surface const* surf, geometry::Point const& new_top_left) override;
    void hidden_set_to(Surface const* surf, bool) override;
    void frame_posted(Surface const* surf, geometry::Rectangle const& damage) override;
    void alpha_set_to(Surface const* surf, float) override;
    void transformation_set_to(Surface const* surf, glm::mat4 const&) override;
    void reception_mode_set_to(Surface const* surf, input::InputReceptionMode mode) override;
    void renamed(Surface const* surf, std::string const&) override;

private:
    std::function<void()> const notify_scene_change;
    std::function<void(geometry::Rectangle const&)> const notify_buffer_change;

    std::mutex mutex;
    geometry::Point top_left;
};
}
}

#endif // MIR_SCENE_SURFACE_CHANGE_NOTIFICATION_H_
