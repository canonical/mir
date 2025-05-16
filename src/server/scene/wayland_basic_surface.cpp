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

#include "wayland_basic_surface.h"

namespace mc = mir::compositor;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace geom = mir::geometry;


ms::WaylandBasicSurface::WaylandBasicSurface(
    std::shared_ptr<Session> const& session,
    wayland::Weak<frontend::WlSurface> wayland_surface,
    std::string const& name,
    geometry::Rectangle rect,
    std::weak_ptr<Surface> const& parent,
    MirPointerConfinementState confinement_state,
    std::list<StreamInfo> const& layers,
    std::shared_ptr<mg::CursorImage> const& cursor_image,
    std::shared_ptr<SceneReport> const& report,
    std::shared_ptr<ObserverRegistrar<mg::DisplayConfigurationObserver>> const& display_config_registrar) :
    BasicSurface(name, rect, parent, confinement_state, layers, cursor_image, report, display_config_registrar),
    session_(session),
    wayland_surface_(wayland_surface)
{
}

ms::WaylandBasicSurface::WaylandBasicSurface(
    std::shared_ptr<Session> const& session,
    wayland::Weak<frontend::WlSurface> wayland_surface,
    std::string const& name,
    geometry::Rectangle rect,
    MirPointerConfinementState state,
    std::list<StreamInfo> const& layers,
    std::shared_ptr<mg::CursorImage> const& cursor_image,
    std::shared_ptr<SceneReport> const& report,
    std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar) :
    BasicSurface(name, rect, state, layers, cursor_image, report, display_config_registrar),
    session_(session),
    wayland_surface_(wayland_surface)
{
}

auto ms::WaylandBasicSurface::session() const -> std::weak_ptr<Session>
{
    return session_;
}

auto ms::WaylandBasicSurface::wayland_surface() -> wayland::Weak<frontend::WlSurface> const&
{
    return wayland_surface_;
}


