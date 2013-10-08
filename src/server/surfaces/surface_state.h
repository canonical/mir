/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_SURFACES_SURFACE_STATE_H_
#define MIR_SURFACES_SURFACE_STATE_H_

#include "mir/compositor/compositing_criteria.h"
#include "mir/input/surface.h"
#include "mutable_surface_state.h"

namespace mir
{
namespace surfaces
{

class SurfaceState : public compositor::CompositingCriteria, public input::Surface,
                     public MutableSurfaceState 
{
protected:
    SurfaceState() = default; 
    virtual ~SurfaceState() = default;
    SurfaceState(const SurfaceState&) = delete;
    SurfaceState& operator=(const SurfaceState& ) = delete;
};

}
}
#endif /* MIR_SURFACES_SURFACE_STATE_H_ */
