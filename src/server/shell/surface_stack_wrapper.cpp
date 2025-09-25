/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/shell/surface_stack_wrapper.h"

#include "mir/geometry/point.h"

namespace msh = mir::shell;


msh::SurfaceStackWrapper::SurfaceStackWrapper(std::shared_ptr<SurfaceStack> const& wrapped) :
    wrapped(wrapped)
{
}

void msh::SurfaceStackWrapper::add_surface(
    std::shared_ptr<scene::Surface> const& surface,
    input::InputReceptionMode new_mode)
{
    wrapped->add_surface(surface, new_mode);
}

void msh::SurfaceStackWrapper::raise(std::weak_ptr<scene::Surface> const& surface)
{
    wrapped->raise(surface);
}

void msh::SurfaceStackWrapper::raise(scene::SurfaceSet const& surfaces)
{
    wrapped->raise(surfaces);
}

void msh::SurfaceStackWrapper::remove_surface(std::weak_ptr<scene::Surface> const& surface)
{
    wrapped->remove_surface(surface);

}

auto msh::SurfaceStackWrapper::surface_at(geometry::Point point) const -> std::shared_ptr<scene::Surface>
{
    return wrapped->surface_at(point);
}

void msh::SurfaceStackWrapper::swap_z_order(scene::SurfaceSet const& first, scene::SurfaceSet const& second)
{
    wrapped->swap_z_order(first, second);
}

void msh::SurfaceStackWrapper::send_to_back(const scene::SurfaceSet &surfaces)
{
    wrapped->send_to_back(surfaces);
}

auto msh::SurfaceStackWrapper::is_above(
    std::weak_ptr<scene::Surface> const& a,
    std::weak_ptr<scene::Surface> const& b) const -> bool
{
    return wrapped->is_above(a, b);
}
