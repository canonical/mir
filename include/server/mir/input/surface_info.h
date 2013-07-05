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

#ifndef MIR_INPUT_SURFACE_INFO_H_
#define MIR_INPUT_SURFACE_INFO_H_

#include <vector>
#include <memory>
#include <string>

namespace mir
{
namespace surfaces
{
class SurfaceInfo;
}

namespace input
{
class SurfaceInfo 
{
public:
    virtual geometry::Rectangle size_and_position() const = 0;
    virtual std::string const& name() const = 0;

    virtual bool input_region_contains(geometry::Point const& point) const = 0;
    virtual void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles) = 0;

protected:
    SurfaceInfo() = default; 
    virtual ~SurfaceInfo() = default;
    SurfaceInfo(const SurfaceInfo&) = delete;
    SurfaceInfo& operator=(const SurfaceInfo& ) = delete;
};

}
}
#endif /* MIR_SURFACES_SURFACE_INFO_H_ */
