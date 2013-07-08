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

namespace mg = mir::graphics;
namespace ms = mir::surfaces;
mg::SurfaceState::SurfaceState(std::shared_ptr<ms::SurfaceInfo> const& basic_info,
                               std::function<void()> change_cb)
    : notify_change(change_cb),
      surface_alpha(1.0f)
{
    (void) basic_info;
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
