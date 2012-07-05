/*
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

namespace mc = mir::compositor;
namespace ms = mir::surfaces;
namespace geometry = mir::geometry;

ms::Surface::Surface(const ms::SurfaceCreationParameters& /*params*/,
                     std::shared_ptr<mc::BufferTextureBinder> buffer_texture_binder) : buffer_texture_binder(buffer_texture_binder)
{
    // TODO(tvoss,kdub): Does a surface without a buffer_bundle make sense?
    assert(buffer_texture_binder);
}

ms::SurfaceCreationParameters::SurfaceCreationParameters(
    geometry::Width w,
    geometry::Height h) : width{w},
    height{h}
    {
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

ms::SurfaceCreationParameters& ms::SurfaceCreationParameters::of_size(geometry::Width new_width, geometry::Height new_height)
{
    width = new_width;
    height = new_height;
        
    return *this;
}

bool ms::SurfaceCreationParameters::operator==(const ms::SurfaceCreationParameters& rhs) const
{
    return width == rhs.width && height == rhs.height;
}

ms::SurfaceCreationParameters ms::a_surface()
{
    return SurfaceCreationParameters{};
}
