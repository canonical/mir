/*
 * Copyright © Canonical Ltd.
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
 */

#include "surface_allocator.h"
#include "mir/scene/buffer_stream_factory.h"
#include "mir/shell/surface_specification.h"
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
    std::shared_ptr<SceneReport> const& report,
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar) :
    default_cursor_image(default_cursor_image),
    report(report),
    display_config_registrar{display_config_registrar}
{
}

std::shared_ptr<ms::Surface> ms::SurfaceAllocator::create_surface(
    std::shared_ptr<Session> const& session,
    wayland::Weak<frontend::WlSurface> const& wayland_surface,
    std::list<ms::StreamInfo> const& streams,
    shell::SurfaceSpecification const& params)
{
    auto confine = params.confine_pointer.is_set() ? params.confine_pointer.value() : mir_pointer_unconfined;
    auto const name = params.name.is_set() ? params.name.value() : "";
    geom::Rectangle const rect{
        params.top_left.is_set() ? params.top_left.value() : geom::Point{}, {
            params.width.is_set() ? params.width.value() : geom::Width{100},
            params.height.is_set() ? params.height.value() : geom::Height{100}}};
    auto const parent = params.parent.is_set() ? params.parent.value() : std::weak_ptr<scene::Surface>{};
    auto const surface = std::make_shared<BasicSurface>(
        session,
        wayland_surface,
        name,
        rect,
        parent,
        confine,
        streams,
        default_cursor_image,
        report,
        display_config_registrar);

    return surface;
}
