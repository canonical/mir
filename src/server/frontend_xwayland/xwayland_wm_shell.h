/*
 * Copyright (C) 2018 Marius Gripsgard <marius@ubports.com>
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
#include <wayland-server-core.h>

namespace mir
{
namespace shell
{
class Shell;
}
namespace frontend
{
class OutputManager;
class WlSeat;
class WlSurface;
class XWaylandSurface;

class XWaylandWMShell
{
public:
    XWaylandWMShell(
        std::shared_ptr<shell::Shell> const& shell,
        WlSeat& seat,
        OutputManager* const output_manager);

    std::shared_ptr<shell::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;
};

} /* frontend*/
} /* mir */

#endif /* end of include guard: MIR_FRONTEND_XWAYLAND_WM_SHELL_H */
