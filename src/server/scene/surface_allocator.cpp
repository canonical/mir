/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "surface_allocator.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/compositor/buffer_stream.h"
#include "basic_surface.h"

namespace geom=mir::geometry;
namespace mc=mir::compositor;
namespace mg=mir::graphics;
namespace ms=mir::scene;
namespace msh=mir::shell;
namespace mi=mir::input;

ms::SurfaceAllocator::SurfaceAllocator(
    std::shared_ptr<mg::CursorImage> const& default_cursor_image,
    std::shared_ptr<SceneReport> const& report) :
    default_cursor_image(default_cursor_image),
    report(report)
{
}

std::shared_ptr<ms::Surface> ms::SurfaceAllocator::create_surface(
    std::shared_ptr<Session> const& session,
    std::list<ms::StreamInfo> const& streams,
    SurfaceCreationParameters const& params)
{
    auto confine = params.confine_pointer.is_set() ? params.confine_pointer.value() : mir_pointer_unconfined;
    auto const surface = std::make_shared<BasicSurface>(
        session,
        params.name,
        geom::Rectangle{params.top_left, params.size},
        params.parent,
        confine,
        streams,
        default_cursor_image,
        report);

    return surface;
}
