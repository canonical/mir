/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/shell/surface_ready_observer.h"
#include "mir/scene/surface.h"

namespace msh = mir::shell;
namespace ms = mir::scene;

msh::SurfaceReadyObserver::SurfaceReadyObserver(
    ActivateFunction const& activate,
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface) :
    activate{activate},
    session{session},
    surface{surface}
{
}

msh::SurfaceReadyObserver::~SurfaceReadyObserver()
    = default;

void msh::SurfaceReadyObserver::frame_posted(ms::Surface const*, int, geometry::Size const&)
{
    if (auto const s = surface.lock())
    {
        activate(session.lock(), s);
        s->remove_observer(shared_from_this());
    }
}
