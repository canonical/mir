/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_FRONTEND_WAYLAND_DELETED_FOR_RESOURCE_H_
#define MIR_FRONTEND_WAYLAND_DELETED_FOR_RESOURCE_H_

#include <memory>

struct wl_resource;

namespace mir
{
namespace frontend
{
namespace wayland
{
std::shared_ptr<bool> deleted_flag_for_resource(wl_resource*resource);
}
}
}

#endif //MIR_FRONTEND_WAYLAND_DELETED_FOR_RESOURCE_H_
