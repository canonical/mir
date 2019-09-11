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

#include <memory>

struct MirInputEvent;

extern "C"
{
struct wl_client;
struct wl_resource;
struct wl_display;
struct wl_global;
struct wl_array;

struct wl_display * wl_client_get_display(struct wl_client *client);
uint32_t wl_display_next_serial(struct wl_display *display);
uint32_t wl_display_get_serial(struct wl_display *display);

void wl_array_init(struct wl_array *array);
void wl_array_release(struct wl_array *array);

#include <wayland-server-core.h>
}

namespace mir
{
namespace frontend
{

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

}
}

#endif // MIR_FRONTEND_WAYLAND_UTILS_H
