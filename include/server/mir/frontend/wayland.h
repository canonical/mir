/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_WAYLAND_H
#define MIR_FRONTEND_WAYLAND_H

#include <memory>
#include <string>
#include <vector>

struct wl_client;
struct wl_resource;

namespace mir
{
namespace frontend
{
class Session;
class Surface;

/// Utility function to recover the session associated with a wl_client
auto get_session(wl_client* client) -> std::shared_ptr<Session>;

/// Utility function to recover the session associated with a wl_resource
auto get_session(wl_resource* resource) -> std::shared_ptr<Session>;

/// Utility function to recover the window associated with a wl_client
auto get_window(wl_resource* surface) -> std::shared_ptr<Surface>;

/// Returns the "standard" extensions Mir recomends enabling (a subset of supported extensions)
auto get_standard_extensions() -> std::vector<std::string>;

/// Returns all extensions core Mir supports
auto get_supported_extensions() -> std::vector<std::string>;
}
}

#endif //MIR_FRONTEND_WAYLAND_H
