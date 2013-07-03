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

#ifndef MIR_SURFACES_SURFACE_INFO_H_
#define MIR_SURFACES_SURFACE_INFO_H_

#include "mir/geometry/point.h"
#include "mir/geometry/size.h"

#include <string>

namespace mir
{
namespace surfaces
{

//TODO: (kdub) as this interface grows, should we split into a read-only interface and a mutator? 
class SurfaceInfo 
{
public:
    virtual geometry::Point top_left() const = 0;
    virtual geometry::Size size() const = 0;
    virtual std::string const& name() const = 0;

    //mutators
    virtual void set_top_left(geometry::Point) = 0;

protected:
    SurfaceInfo() = default; 
    virtual ~SurfaceInfo() = default;
    SurfaceInfo(const SurfaceInfo&) = delete;
    SurfaceInfo& operator=(const SurfaceInfo& ) = delete;
};

}
}
#endif /* MIR_SURFACES_SURFACE_INFO_H_ */
