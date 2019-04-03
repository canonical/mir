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

struct wl_client;

namespace mir
{
namespace frontend
{
class Session;

/// Utility function to recover the session associated with a wl_client
auto get_session(wl_client* client) -> std::shared_ptr<Session>;

}
}

#endif //MIR_FRONTEND_WAYLAND_H
