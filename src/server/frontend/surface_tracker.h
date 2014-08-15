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

#ifndef MIR_FRONTEND_SURFACE_TRACKER_H_
#define MIR_FRONTEND_SURFACE_TRACKER_H_

#include "mir/frontend/surface_id.h"
#include "client_buffer_tracker.h"
#include <unordered_map>
#include <memory>

namespace mir
{
namespace graphics
{
class Buffer;
}
namespace frontend
{

class SurfaceTracker
{
public:
    SurfaceTracker(size_t client_cache_size);
    SurfaceTracker(SurfaceTracker const&) = delete;
    SurfaceTracker& operator=(SurfaceTracker const&) = delete;

    /* track a buffer as associated with a surface 
     * \param surface_id id that the the buffer is associated with
     * \param buffer     buffer to be tracked (TODO: should be a shared_ptr)
     * \returns          true if the buffer is already tracked
     *                   false if the buffer is not tracked
     */
    bool track_buffer(SurfaceId, graphics::Buffer*);
    /* removes the surface id from all tracking */
    void remove_surface(SurfaceId);
    /* accesses the last buffer given to track_buffer() for the given SurfaceId */
    graphics::Buffer* last_buffer(SurfaceId) const;
private:
    size_t const client_cache_size;

    std::unordered_map<SurfaceId, graphics::Buffer*> client_buffer_resource;
    std::unordered_map<SurfaceId, std::shared_ptr<ClientBufferTracker>> client_buffer_tracker;
};

}
}

#endif // MIR_FRONTEND_SURFACE_TRACKER_H_
