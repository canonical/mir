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

#include "mir/graphics/buffer.h"
#include "mir/frontend/surface_id.h"

namespace mir
{
namespace frontend
{
class SurfaceTracker
{
public:
    virtual void add_buffer_to_surface(SurfaceId, graphics::Buffer*) = 0;
    virtual void remove_surface(SurfaceId) = 0;
    virtual graphics::Buffer* last_buffer(SurfaceId) const = 0;
    virtual bool surface_has_buffer(SurfaceId, graphics::Buffer*) const = 0;
    virtual ~SurfaceTracker() = default;
    SurfaceTracker() = default;
private:
    SurfaceTracker(SurfaceTracker const&) = delete;
    SurfaceTracker& operator=(SurfaceTracker const&) = delete;
};

}
}

#endif // MIR_FRONTEND_SURFACE_TRACKER_H_
