/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "surface_source.h"
#include "surface_builder.h"

#include "mir/scene/surface.h"

#include <cstdlib>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mf = mir::frontend;

ms::SurfaceSource::SurfaceSource(std::shared_ptr<SurfaceBuilder> const& surface_builder)
    : surface_builder(surface_builder)
{
}

std::shared_ptr<msh::Surface> ms::SurfaceSource::create_surface(
    msh::Session* /*session*/,
    shell::SurfaceCreationParameters const& params,
    frontend::SurfaceId id,
    std::shared_ptr<mf::EventSink> const& sender)
{
    return surface_builder->create_surface(id, params, sender);
}

void ms::SurfaceSource::destroy_surface(std::shared_ptr<msh::Surface> const& surface)
{
    if (auto const scene_surface = std::dynamic_pointer_cast<Surface>(surface))
    {
        surface_builder->destroy_surface(scene_surface);
    }
    else
    {
        // We shouldn't be destroying surfaces we didn't create,
        // so we ought to be able to restore the original type!
        std::abort();
    }
}
