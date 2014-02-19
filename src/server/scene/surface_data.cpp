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

#include "surface_data.h"
#include <glm/gtc/matrix_transform.hpp>

namespace geom=mir::geometry;
namespace ms = mir::scene;

ms::SurfaceData::SurfaceData(std::string const& name, geom::Rectangle rect, std::function<void()> change_cb, bool nonrectangular)
    : notify_change(change_cb),
      surface_name(name),
      surface_rect(rect),
      transformation_dirty(true),
      surface_alpha(1.0f),
      first_frame_posted(false),
      hidden(false),
      nonrectangular(nonrectangular),
      input_rectangles{surface_rect}
{
}

float ms::SurfaceData::alpha() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_alpha;
}

glm::mat4 const& ms::SurfaceData::transformation() const
{
    std::unique_lock<std::mutex> lk(guard);

    auto surface_size = surface_rect.size;
    auto surface_top_left = surface_rect.top_left;
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

bool ms::SurfaceData::should_be_rendered_in(geom::Rectangle const& rect) const
{
    std::unique_lock<std::mutex> lk(guard);

    if (hidden || !first_frame_posted)
        return false;

    return rect.overlaps(surface_rect);
}

bool ms::SurfaceData::shaped() const
{
    return nonrectangular;
}

void ms::SurfaceData::apply_alpha(float alpha)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        surface_alpha = alpha;
    }
    notify_change();
}


void ms::SurfaceData::apply_rotation(float degrees, glm::vec3 const& axis)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        rotation_matrix = glm::rotate(glm::mat4{1.0f}, degrees, axis);
        transformation_dirty = true;
    }
    notify_change();
}

void ms::SurfaceData::frame_posted()
{
    {
        std::unique_lock<std::mutex> lk(guard);
        first_frame_posted = true;
    }
    notify_change();
}

void ms::SurfaceData::set_hidden(bool hide)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        hidden = hide;
    }
    notify_change();
}

geom::Point ms::SurfaceData::position() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_rect.top_left;
}

geom::Size ms::SurfaceData::size() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_rect.size;
}

std::string const& ms::SurfaceData::name() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_name;
}

void ms::SurfaceData::move_to(geom::Point new_pt)
{
    {
        std::unique_lock<std::mutex> lk(guard);
        surface_rect.top_left = new_pt;
        transformation_dirty = true;
    }
    notify_change();
}

void ms::SurfaceData::resize(geom::Size const& size)
{
    {
        std::unique_lock<std::mutex> lock(guard);
        surface_rect.size = size;
        transformation_dirty = true;
    }
    notify_change();
}

bool ms::SurfaceData::contains(geom::Point const& point) const
{
    std::unique_lock<std::mutex> lock(guard);
    if (hidden)
        return false;

    for (auto const& rectangle : input_rectangles)
    {
        if (rectangle.contains(point))
        {
            return true;
        }
    }
    return false;
}

void ms::SurfaceData::set_input_region(std::vector<geom::Rectangle> const& rectangles)
{
    std::unique_lock<std::mutex> lock(guard);
    input_rectangles = rectangles;
}
