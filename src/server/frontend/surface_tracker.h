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
    /* track a buffer as associated with a surface 
     * \param surface_id id that the the buffer is associated with
     * \param buffer     buffer to be tracked (TODO: should be a shared_ptr)
     * \returns          true if the buffer is already tracked
     *                   false if the buffer is not tracked
     */
    virtual bool track_buffer(SurfaceId, graphics::Buffer*) = 0;
    /* removes the surface id from all tracking */
    virtual void remove_surface(SurfaceId) = 0;
    /* accesses the last buffer given to track_buffer() for the given SurfaceId */
    virtual graphics::Buffer* last_buffer(SurfaceId) const = 0;

    virtual ~SurfaceTracker() = default;
    SurfaceTracker() = default;
private:
    SurfaceTracker(SurfaceTracker const&) = delete;
    SurfaceTracker& operator=(SurfaceTracker const&) = delete;
};

}
}

#endif // MIR_FRONTEND_SURFACE_TRACKER_H_
