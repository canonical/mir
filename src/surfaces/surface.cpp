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
#include "mir/compositor/buffer_bundle.h"

#include <cassert>

namespace mc = mir::compositor;
namespace ms = mir::surfaces;

ms::Surface::Surface(
    const std::string& name,
    std::shared_ptr<mc::BufferBundle> buffer_bundle) :
    surface_name(name),
    buffer_bundle(buffer_bundle)
{
    // TODO(tvoss,kdub): Does a surface without a buffer_bundle make sense?
    assert(buffer_bundle);
}

std::string const& ms::Surface::name() const
{
    return surface_name;
}

mir::geometry::Width ms::Surface::width() const
{
    return buffer_bundle->bundle_width();
}

mir::geometry::Height ms::Surface::height() const
{
    return buffer_bundle->bundle_height();
}

//note: not sure the surface should be aware of pixel format. might be something that the 
//texture (which goes to compositor should be aware of though
//todo: kdub remove 
mc::PixelFormat ms::Surface::pixel_format() const
{
    return buffer_bundle->get_bundle_pixel_format();
}

std::shared_ptr<mc::BufferIPCPackage> ms::Surface::get_buffer_ipc_package() const
{
    auto graphics_resource = buffer_bundle->secure_client_buffer();
    return graphics_resource->ipc_package;
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
