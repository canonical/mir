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

#ifndef MIR_FRONTEND_XDG_TOPLEVEL_V6_H
#define MIR_FRONTEND_XDG_TOPLEVEL_V6_H

#include "xdg_shell_generated_interfaces.h"

namespace mir
{
namespace frontend
{

class XdgSurfaceV6;
class Shell;

class XdgToplevelV6 : public wayland::XdgToplevelV6
{
public:
    XdgToplevelV6(struct wl_client* client, struct wl_resource* parent, uint32_t id,
                  std::shared_ptr<frontend::Shell> const& shell, XdgSurfaceV6* self);

    void destroy() override;
    void set_parent(std::experimental::optional<struct wl_resource*> const& parent) override;
    void set_title(std::string const& title) override;
    void set_app_id(std::string const& /*app_id*/) override;
    void show_window_menu(struct wl_resource* seat, uint32_t serial, int32_t x, int32_t y) override;
    void move(struct wl_resource* seat, uint32_t serial) override;
    void resize(struct wl_resource* seat, uint32_t serial, uint32_t edges) override;
    void set_max_size(int32_t width, int32_t height) override;
    void set_min_size(int32_t width, int32_t height) override;
    void set_maximized() override;
    void unset_maximized() override;
    void set_fullscreen(std::experimental::optional<struct wl_resource*> const& output) override;
    void unset_fullscreen() override;
    void set_minimized() override;

private:
    XdgToplevelV6* get_xdgtoplevel(wl_resource* surface) const;

    std::shared_ptr<frontend::Shell> const shell;
    XdgSurfaceV6* const self;
};

}
}

#endif // MIR_FRONTEND_XDG_TOPLEVEL_V6_H
