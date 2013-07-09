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

#ifndef MIR_INPUT_SURFACE_DATA_STORAGE_H_
#define MIR_INPUT_SURFACE_DATA_STORAGE_H_

#include "mir/input/surface_info.h"

#include <memory>

namespace mir
{
namespace surfaces
{
class SurfaceInfo;
}
namespace input
{

class SurfaceDataStorage : public SurfaceInfoController 
{
public:
    SurfaceDataStorage(std::shared_ptr<surfaces::SurfaceInfo> const& surface_info);

    geometry::Rectangle size_and_position() const;
    std::string const& name() const;

    bool input_region_contains(geometry::Point const& point) const;
    void set_input_region(std::vector<geometry::Rectangle> const& input_rectangles);

private:
    std::shared_ptr<surfaces::SurfaceInfo> surface_info;
    std::vector<geometry::Rectangle> input_rectangles;
};

}
}
#endif /* MIR_SURFACES_SURFACE_DATA_STORAGE_H_ */
