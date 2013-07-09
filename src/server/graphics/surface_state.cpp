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

#include "mir/graphics/surface_state.h"
#include "mir/surfaces/surface_info.h"

namespace mg = mir::graphics;
namespace ms = mir::surfaces;

mg::SurfaceState::SurfaceState(std::shared_ptr<ms::SurfaceInfo> const& basic_info,
                               std::function<void()> change_cb)
    : basic_info(basic_info),
      notify_change(change_cb),
      surface_alpha(1.0f)
{
}

void mg::SurfaceState::apply_alpha(float alpha)
{
    std::unique_lock<std::mutex> lk(guard);
    surface_alpha = alpha;
    notify_change();
}

float mg::SurfaceState::alpha() const
{
    std::unique_lock<std::mutex> lk(guard);
    return surface_alpha;
}

void mg::SurfaceState::apply_rotation(float degrees, glm::vec3 const& mat)
{
    basic_info->apply_rotation(degrees, mat);
    notify_change();
}

glm::mat4 const& mg::SurfaceState::transformation() const
{
    return basic_info->transformation(); 
}
