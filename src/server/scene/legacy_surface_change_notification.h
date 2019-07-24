/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SCENE_LEGACY_SURFACE_CHANGE_NOTIFICATION_H_
#define MIR_SCENE_LEGACY_SURFACE_CHANGE_NOTIFICATION_H_

#include "mir/scene/null_surface_observer.h"

#include <functional>

namespace mir
{
namespace scene
{
class LegacySurfaceChangeNotification : public mir::scene::NullSurfaceObserver
{
public:
    LegacySurfaceChangeNotification(
        std::function<void()> const& notify_scene_change,
        std::function<void(int)> const& notify_buffer_change);

    void resized_to(Surface const* surf, geometry::Size const&) override;
    void moved_to(Surface const* surf, geometry::Point const&) override;
    void hidden_set_to(Surface const* surf, bool) override;
    void frame_posted(Surface const* surf, int frames_available, geometry::Size const& size) override;
    void alpha_set_to(Surface const* surf, float) override;
    void transformation_set_to(Surface const* surf, glm::mat4 const&) override;
    void reception_mode_set_to(Surface const* surf, input::InputReceptionMode mode) override;
    void renamed(Surface const* surf, char const*) override;

private:
    std::function<void()> const notify_scene_change;
    std::function<void(int)> const notify_buffer_change;
};
}
}

#endif // MIR_SCENE_LEGACY_SURFACE_CHANGE_NOTIFICATION_H_
