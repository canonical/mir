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

#ifndef MIR_FRONTEND_WAYLAND_WL_SHELL_H_
#define MIR_FRONTEND_WAYLAND_WL_SHELL_H_

#include "wayland.h"
#include "client.h"
#include <memory>

namespace mir
{
class Executor;
namespace shell
{
class Shell;
}
namespace scene
{
class Surface;
}
namespace frontend
{
class WlSeat;
class OutputManager;
class SurfaceRegistry;

class WlShell : public wayland_rs::WlShellImpl
{
public:
    WlShell(
        std::shared_ptr<wayland_rs::Client> const& client,
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> const& shell,
        WlSeat& seat,
        OutputManager* const output_manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry);

    auto get_shell_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface) -> std::shared_ptr<wayland_rs::WlShellSurfaceImpl> override;

private:
    std::shared_ptr<wayland_rs::Client> client;
    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;
    std::shared_ptr<SurfaceRegistry> const surface_registry;
};

auto get_wl_shell_window(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface) -> std::shared_ptr<scene::Surface>;
}
}

#endif // MIR_FRONTEND_WAYLAND_WL_SHELL_H_
