/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_XDG_SHELL_V6_H
#define MIR_FRONTEND_XDG_SHELL_V6_H

#include "xdg-shell-unstable-v6_wrapper.h"

namespace mir
{
class Executor;
namespace scene
{
class Surface;
}
namespace shell
{
class Shell;
}
namespace frontend
{
class WlSeat;
class OutputManager;

class XdgShellV6 : public wayland::XdgShellV6::Global
{
public:
    XdgShellV6(
        wl_display* display,
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> const& shell,
        WlSeat& seat,
        OutputManager* output_manager);

    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;

    // Returns the Mir surface if the window is recognised by XdgShellV6
    static auto get_window(wl_resource* surface) -> std::shared_ptr<scene::Surface>;

private:
    class Instance;
    void bind(wl_resource* new_resource) override;
};

}
}

#endif // MIR_FRONTEND_XDG_SHELL_V6_H
