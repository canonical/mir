/*
 * Copyright (C) Marius Gripsgard <marius@ubports.com>
 * Copyright (C) Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_XWAYLAND_WM_SHELL_H
#define MIR_FRONTEND_XWAYLAND_WM_SHELL_H

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
class Clipboard;
}
namespace frontend
{
class OutputManager;
class SurfaceStack;
class WlSeat;
class WlSurface;
class XWaylandSurface;
class SessionAuthorizer;
class SurfaceRegistry;

class XWaylandWMShell
{
public:
    XWaylandWMShell(
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<shell::Shell> const& shell,
        std::shared_ptr<SessionAuthorizer> const& session_authorizer,
        std::shared_ptr<scene::Clipboard> const& clipboard,
        WlSeat& seat,
        std::shared_ptr<SurfaceStack> const& surface_stack,
        std::shared_ptr<SurfaceRegistry> const& surface_registry)
        : wayland_executor{wayland_executor},
          shell{shell},
          session_authorizer{session_authorizer},
          clipboard{clipboard},
          seat{seat},
          surface_stack{surface_stack},
          surface_registry{surface_registry}
    {
    }

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<SessionAuthorizer> const session_authorizer;
    std::shared_ptr<scene::Clipboard> const clipboard;
    WlSeat& seat;
    std::shared_ptr<SurfaceStack> const surface_stack;
    std::shared_ptr<SurfaceRegistry> const surface_registry;
};

} /* frontend*/
} /* mir */

#endif // MIR_FRONTEND_XWAYLAND_WM_SHELL_H
