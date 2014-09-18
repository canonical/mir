/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_SCENE_RENDERING_TRACKER_H_
#define MIR_SCENE_RENDERING_TRACKER_H_

#include "mir/compositor/compositor_id.h"

#include <memory>
#include <set>
#include <mutex>

#include "mir_toolkit/common.h"

namespace mir
{
namespace scene
{

class Surface;

class RenderingTracker
{
public:
    RenderingTracker(std::weak_ptr<Surface> const& weak_surface);

    void rendered_in(compositor::CompositorID cid);
    void occluded_in(compositor::CompositorID cid);
    void active_compositors(std::set<compositor::CompositorID> const& cids);

private:
    bool occluded_in_all_active_compositors();
    void configure_visibility(MirSurfaceVisibility visibility);
    void remove_occlusions_for_inactive_compositors();
    void ensure_is_active_compositor(compositor::CompositorID cid) const;

    std::weak_ptr<Surface> const weak_surface;
    std::set<compositor::CompositorID> occlusions;
    std::set<compositor::CompositorID> active_compositors_;
    std::mutex guard;
};

}
}

#endif
