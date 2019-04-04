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

#ifndef MIR_FRONTEND_XDG_SHELL_V6_H
#define MIR_FRONTEND_XDG_SHELL_V6_H

#include "xdg-shell-unstable-v6_wrapper.h"

namespace mir
{
namespace frontend
{

class Shell;
class Surface;
class WlSeat;
class OutputManager;

class XdgShellV6 : public wayland::XdgShellV6
{
public:
    XdgShellV6(struct wl_display* display, std::shared_ptr<Shell> const shell, WlSeat& seat, OutputManager* output_manager);

    void destroy(struct wl_client* client, struct wl_resource* resource) override;
    void create_positioner(struct wl_client* client, struct wl_resource* resource, uint32_t id) override;
    void get_xdg_surface(struct wl_client* client, struct wl_resource* resource, uint32_t id,
                         struct wl_resource* surface) override;
    void pong(struct wl_client* client, struct wl_resource* resource, uint32_t serial) override;

    std::shared_ptr<Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;

    // Returns the Mir surface if the window is recognised by XdgShellV6
    static auto get_window(wl_resource* window) -> std::shared_ptr<Surface>;
};

}
}

#endif // MIR_FRONTEND_XDG_SHELL_V6_H
