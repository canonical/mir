/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Alan Griffiths <alan@octopull.co.uk>
 *   Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/surfaces/surface.h"
#include "mir/compositor/buffer_ipc_package.h"

#include <cassert>

namespace mc = mir::compositor;
namespace ms = mir::surfaces;

ms::Surface::Surface(
    const ms::SurfaceCreationParameters& params,
    std::shared_ptr<mc::BufferTextureBinder> buffer_texture_binder) :
    params(params),
    buffer_texture_binder(buffer_texture_binder)
{
    // TODO(tvoss,kdub): Does a surface without a buffer_bundle make sense?
    assert(buffer_texture_binder);
}

std::string const& ms::Surface::name() const
{
    return params.name;
}

mir::geometry::Width ms::Surface::width() const
{
    return params.width;
}

mir::geometry::Height ms::Surface::height() const
{
    return params.height;
}

mc::PixelFormat ms::Surface::pixel_format() const
{
    // TODO This should actually be supplied from somewhere (where?)
    return mc::PixelFormat();
}

std::shared_ptr<mc::BufferIPCPackage> ms::Surface::get_current_buffer_ipc_package() const
{
    // TODO This should actually be supplied from somewhere (where?)
    return std::make_shared<mc::BufferIPCPackage>();
}


std::shared_ptr<mc::BufferIPCPackage> ms::Surface::get_next_buffer_ipc_package() const
{
    // TODO Should advance the buffer
    // TODO This should actually be supplied from somewhere (where?)
    return std::make_shared<mc::BufferIPCPackage>();
}


ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_name(std::string const& new_name)
{
    name = new_name;
    return *this;
}


ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_width(geometry::Width new_width)
{
    width = new_width;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_height(geometry::Height new_height)
{
    height = new_height;
    return *this;
}

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_size(
        geometry::Width new_width,
        geometry::Height new_height)
{
    width = new_width;
    height = new_height;

    return *this;
}

bool ms::operator==(
    const SurfaceCreationParameters& lhs,
    const ms::SurfaceCreationParameters& rhs)
{
    return lhs.width == rhs.width && lhs.height == rhs.height;
}

bool ms::operator!=(
    const SurfaceCreationParameters& lhs,
    const ms::SurfaceCreationParameters& rhs)
{
    return lhs.width != rhs.width || lhs.height != rhs.height;
}

ms::SurfaceCreationParameters ms::a_surface()
{
    return SurfaceCreationParameters();
}
