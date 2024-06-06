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

#include "rendering_tracker.h"
#include "mir/scene/surface.h"

#include <algorithm>
#include <iterator>
#include <stdexcept>
#include <boost/throw_exception.hpp>

namespace ms = mir::scene;
namespace mc = mir::compositor;

ms::RenderingTracker::RenderingTracker(
    std::weak_ptr<ms::Surface> const& weak_surface)
    : weak_surface{weak_surface}
{
}

void ms::RenderingTracker::rendered_in(mc::CompositorID cid)
{
    std::lock_guard lock{guard};

    ensure_is_active_compositor(cid);

    occlusions.erase(cid);

    configure_visibility(mir_window_visibility_exposed);
}

void ms::RenderingTracker::occluded_in(mc::CompositorID cid)
{
    std::lock_guard lock{guard};

    ensure_is_active_compositor(cid);

    occlusions.insert(cid);

    if (occluded_in_all_active_compositors())
        configure_visibility(mir_window_visibility_occluded);
}

void ms::RenderingTracker::active_compositors(std::set<mc::CompositorID> const& cids)
{
    std::lock_guard lock{guard};

    active_compositors_ = cids;

    remove_occlusions_for_inactive_compositors();

    if (occluded_in_all_active_compositors())
        configure_visibility(mir_window_visibility_occluded);
}

bool ms::RenderingTracker::is_exposed_in(mc::CompositorID cid) const
{
    std::lock_guard lock{guard};

    ensure_is_active_compositor(cid);

    if (occlusions.size() == 0)
        return true;

    return occlusions.find(cid) == occlusions.end();
}

bool ms::RenderingTracker::occluded_in_all_active_compositors()
{
    return occlusions == active_compositors_;
}

void ms::RenderingTracker::configure_visibility(MirWindowVisibility visibility)
{
    if (auto const surface = weak_surface.lock())
        surface->configure(mir_window_attrib_visibility, visibility);
}

void ms::RenderingTracker::remove_occlusions_for_inactive_compositors()
{
    std::set<mc::CompositorID> new_occlusions;

    std::set_intersection(
        active_compositors_.begin(), active_compositors_.end(),
        occlusions.begin(), occlusions.end(),
        std::inserter(new_occlusions, new_occlusions.begin()));

    occlusions = std::move(new_occlusions);
}

void ms::RenderingTracker::ensure_is_active_compositor(compositor::CompositorID cid) const
{
    if (active_compositors_.find(cid) == active_compositors_.end())
        BOOST_THROW_EXCEPTION(std::logic_error("No active compositor with supplied id"));
}
