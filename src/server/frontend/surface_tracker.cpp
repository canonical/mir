/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "surface_tracker.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/buffer_id.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;

mf::SurfaceTracker::SurfaceTracker(size_t client_cache_size) :
    client_cache_size{client_cache_size}
{
}

bool mf::SurfaceTracker::track_buffer(SurfaceId surface_id, mg::Buffer* buffer)
{
    auto& tracker = client_buffer_tracker[surface_id];
    if (!tracker)
        tracker = std::make_shared<ClientBufferTracker>(client_cache_size);

    auto already_tracked = tracker->client_has(buffer->id());
    tracker->add(buffer->id());

    client_buffer_resource[surface_id] = buffer;
    return already_tracked;
}

void mf::SurfaceTracker::remove_surface(SurfaceId surface_id)
{
    auto it = client_buffer_tracker.find(surface_id);
    if (it != client_buffer_tracker.end())
        client_buffer_tracker.erase(it);

    auto last_buffer_it = client_buffer_resource.find(surface_id);
    if (last_buffer_it != client_buffer_resource.end())
        client_buffer_resource.erase(last_buffer_it);
}

mg::Buffer* mf::SurfaceTracker::last_buffer(SurfaceId surface_id) const
{
    auto it = client_buffer_resource.find(surface_id);
    if (it != client_buffer_resource.end())
        return it->second;
    else
        //should really throw, but that is difficult with the way the code currently works
        return nullptr;
}
