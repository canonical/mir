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

#ifndef MIR_SURFACES_SURFACE_DATA_STORAGE_H_
#define MIR_SURFACES_SURFACE_DATA_STORAGE_H_

#include "mir/surfaces/surface_info.h"
#include <mutex>

namespace mir
{
namespace surfaces
{

class SurfaceDataStorage : public SurfaceInfoController 
{
public:
    SurfaceDataStorage(std::string const& name, geometry::Point top_left, geometry::Size size);

    geometry::Rectangle size_and_position() const;
    std::string const& name() const;
    void set_top_left(geometry::Point);
    glm::mat4 const& transformation() const;
    void apply_rotation(float degrees, glm::vec3 const&);
private:
    std::mutex mutable guard;

    std::string surface_name;
    glm::mat4 rotation_matrix;
    mutable glm::mat4 transformation_matrix;
    mutable geometry::Size transformation_size;
    mutable bool transformation_dirty;
    geometry::Point surface_top_left;
    geometry::Size surface_size;
};

}
}
#endif /* MIR_SURFACES_SURFACE_DATA_STORAGE_H_ */
