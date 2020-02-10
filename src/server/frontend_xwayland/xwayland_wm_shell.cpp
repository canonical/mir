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

#include "xwayland_wm_shell.h"

#include "mir/log.h"

#include "mir/frontend/shell.h"
#include "wl_seat.h"
#include "wl_surface.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

mf::XWaylandWMShell::XWaylandWMShell(
    std::shared_ptr<msh::Shell> const& shell,
    mf::WlSeat& seat,
    OutputManager* const output_manager)
    : shell{shell},
      seat{seat},
      output_manager{output_manager}
{
}
