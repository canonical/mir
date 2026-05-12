/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_FRONTEND_FOREIGN_TOPLEVEL_HANDLE_CREATION_H
#define MIR_FRONTEND_FOREIGN_TOPLEVEL_HANDLE_CREATION_H

#include "mir/scene/surface.h"

#include <mir_toolkit/common.h>

#include <string>

namespace mir::frontend
{
inline auto should_create_foreign_toplevel_handle(
    scene::Surface const& surface,
    std::string const& app_id) -> bool
{
    auto const type = surface.type();
    auto const is_supported_window_type =
        type == mir_window_type_normal ||
        type == mir_window_type_utility ||
        type == mir_window_type_freestyle;
    auto const has_session = static_cast<bool>(surface.session().lock());
    auto const is_application_layer = surface.depth_layer() == mir_depth_layer_application;
    auto const can_focus = surface.focus_mode() != mir_focus_mode_disabled;
    return has_session && is_application_layer && can_focus && is_supported_window_type && !app_id.empty();
}
}

#endif // MIR_FRONTEND_FOREIGN_TOPLEVEL_HANDLE_CREATION_H
