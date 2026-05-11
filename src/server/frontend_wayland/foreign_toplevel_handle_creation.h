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

namespace mir::frontend
{
/**
 * Returns true when a surface should be exposed via the foreign-toplevel protocols.
 *
 * Eligibility requires a live session, and only includes
 * focusable application-layer normal/utility/freestyle windows.
 */
inline auto should_create_foreign_toplevel_handle(
    scene::Surface const& surface) -> bool
{
    if (!surface.session().lock())
        return false;

    auto const type = surface.type();
    if (type != mir_window_type_normal &&
        type != mir_window_type_utility &&
        type != mir_window_type_freestyle)
        return false;

    if (surface.depth_layer() != mir_depth_layer_application)
        return false;

    return surface.focus_mode() != mir_focus_mode_disabled;
}
}

#endif // MIR_FRONTEND_FOREIGN_TOPLEVEL_HANDLE_CREATION_H
