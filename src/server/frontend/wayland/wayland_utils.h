/*
 * Copyright © 2015-2018 Canonical Ltd.
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

#ifndef MIR_FRONTEND_WAYLAND_UTILS_H
#define MIR_FRONTEND_WAYLAND_UTILS_H

#include "wayland_utils.h"

#include <memory>

struct wl_client;

namespace mir
{
namespace frontend
{
class Session;

template<typename Callable>
inline auto run_unless(std::shared_ptr<bool> const& condition, Callable&& callable)
{
    return [callable = std::move(callable), condition]()
        {
            if (*condition)
                return;
            callable();
        };
}

std::shared_ptr<frontend::Session> get_session(wl_client* client);

}
}

#endif // MIR_FRONTEND_WAYLAND_UTILS_H
