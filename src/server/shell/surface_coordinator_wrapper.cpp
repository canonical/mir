/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/shell/surface_coordinator_wrapper.h"

namespace ms = mir::scene;
namespace msh = mir::shell;


msh::SurfaceCoordinatorWrapper::SurfaceCoordinatorWrapper(
    std::shared_ptr<ms::SurfaceCoordinator> const& wrapped) :
    wrapped(wrapped)
{
}

std::shared_ptr<ms::Surface> msh::SurfaceCoordinatorWrapper::add_surface(
    ms::SurfaceCreationParameters const& params,
    ms::Session* session)
{
    return wrapped->add_surface(params, session);
}

void msh::SurfaceCoordinatorWrapper::raise(std::weak_ptr<ms::Surface> const& surface)
{
    wrapped->raise(surface);
}

void msh::SurfaceCoordinatorWrapper::remove_surface(std::weak_ptr<ms::Surface> const& surface)
{
    wrapped->remove_surface(surface);
}
