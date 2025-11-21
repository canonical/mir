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

#include "wayland_wrapper.h"

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

auto create_wl_shell(
    wl_display* display,
    Executor& wayland_executor,
    std::shared_ptr<shell::Shell> const& shell,
    WlSeat* seat,
    OutputManager* const output_manager,
    std::shared_ptr<SurfaceRegistry> const& surface_registry) -> std::shared_ptr<wayland::Shell::Global>;

auto get_wl_shell_window(wl_resource* surface) -> std::shared_ptr<scene::Surface>;
}
}

#endif // MIR_FRONTEND_WAYLAND_WL_SHELL_H_
