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

#include <mir_toolkit/common.h>

#include <string>

namespace mir::frontend
{
inline auto foreign_toplevel_app_id(std::string const& application_id, std::string const& resolved_app_id) -> std::string
{
    if (!application_id.empty())
        return application_id;

    return resolved_app_id;
}

inline auto should_create_foreign_toplevel_handle(
    MirWindowType type,
    bool has_session,
    std::string const& app_id) -> bool
{
    return has_session && type == mir_window_type_normal && !app_id.empty();
}
}

#endif // MIR_FRONTEND_FOREIGN_TOPLEVEL_HANDLE_CREATION_H
