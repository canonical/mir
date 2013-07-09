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

#include "mir/surfaces/surface_data_storage.h"
#include <glm/gtc/matrix_transform.hpp>

namespace geom=mir::geometry;
namespace ms = mir::surfaces;

ms::SurfaceDataStorage::SurfaceDataStorage(
    std::string const& name, geom::Point top_left, geom::Size size, std::function<void()> change_cb)
    : notify_change(change_cb),
      surface_name(name),
      transformation_dirty(true),
      surface_top_left(top_left),
      surface_size(size)
{
}

geom::Rectangle ms::SurfaceDataStorage::size_and_position() const
{
    std::unique_lock<std::mutex> lk(guard);
    return geom::Rectangle{surface_top_left, surface_size};
}

std::string const& ms::SurfaceDataStorage::name() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_name;
}

glm::mat4 const& ms::SurfaceDataStorage::transformation() const
{
    std::unique_lock<std::mutex> lk(guard);

    if (transformation_dirty || transformation_size != surface_size)
    {
        const glm::vec3 top_left_vec{surface_top_left.x.as_int(),
                                     surface_top_left.y.as_int(),
                                     0.0f};
        const glm::vec3 size_vec{surface_size.width.as_uint32_t(),
                                 surface_size.height.as_uint32_t(),
                                 0.0f};

        /* Get the center of the renderable's area */
        const glm::vec3 center_vec{top_left_vec + 0.5f * size_vec};

        /*
         * Every renderable is drawn using a 1x1 quad centered at 0,0.
         * We need to transform and scale that quad to get to its final position
         * and size.
         *
         * 1. We scale the quad vertices (from 1x1 to wxh)
         * 2. We move the quad to its final position. Note that because the quad
         *    is centered at (0,0), we need to translate by center_vec, not
         *    top_left_vec.
         */
        glm::mat4 pos_size_matrix;
        pos_size_matrix = glm::translate(pos_size_matrix, center_vec);
        pos_size_matrix = glm::scale(pos_size_matrix, size_vec);

        // Rotate, then scale, then translate
        transformation_matrix = pos_size_matrix * rotation_matrix;
        transformation_size = surface_size;
        transformation_dirty = false;
    }

    return transformation_matrix;
}

void ms::SurfaceDataStorage::set_top_left(geom::Point new_pt)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        surface_top_left = new_pt;
        transformation_dirty = true;
    }
    notify_change();
}

void ms::SurfaceDataStorage::apply_rotation(float degrees, glm::vec3 const& axis)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        rotation_matrix = glm::rotate(glm::mat4{1.0f}, degrees, axis);
        transformation_dirty = true;
    }
    notify_change();
}

