/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
 * Copyright (C) 2020 Canonical Ltd.
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
namespace shell
{
class Shell;
}
namespace frontend
{
class OutputManager;
class SurfaceStack;
class WlSeat;
class WlSurface;
class XWaylandSurface;

class XWaylandWMShell
{
public:
    XWaylandWMShell(
        std::shared_ptr<shell::Shell> const& shell,
        WlSeat& seat,
        std::shared_ptr<SurfaceStack> const& surface_stack)
        : shell{shell},
          seat{seat},
          surface_stack{surface_stack}
    {
    }

    std::shared_ptr<shell::Shell> const shell;
    WlSeat& seat;
    std::shared_ptr<SurfaceStack> const surface_stack;
};

} /* frontend*/
} /* mir */

#endif // MIR_FRONTEND_XWAYLAND_WM_SHELL_H
