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

#ifndef MIR_SCENE_WAYLAND_BASIC_SURFACE_H
#define MIR_SCENE_WAYLAND_BASIC_SURFACE_H

#include "mir/scene/basic_surface.h"

namespace mir
{
namespace scene
{
class WaylandBasicSurface : public BasicSurface
{
public:
    WaylandBasicSurface(
        std::shared_ptr<Session> const& session,
        wayland::Weak<frontend::WlSurface> wayland_surface,
        std::string const& name,
        geometry::Rectangle rect,
        MirPointerConfinementState state,
        std::list<scene::StreamInfo> const& streams,
        std::shared_ptr<graphics::CursorImage> const& cursor_image,
        std::shared_ptr<SceneReport> const& report,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar);

    WaylandBasicSurface(
        std::shared_ptr<Session> const& session,
        wayland::Weak<frontend::WlSurface> wayland_surface,
        std::string const& name,
        geometry::Rectangle rect,
        std::weak_ptr<Surface> const& parent,
        MirPointerConfinementState state,
        std::list<scene::StreamInfo> const& streams,
        std::shared_ptr<graphics::CursorImage> const& cursor_image,
        std::shared_ptr<SceneReport> const& report,
        std::shared_ptr<ObserverRegistrar<graphics::DisplayConfigurationObserver>> const& display_config_registrar);

    auto wayland_surface() -> wayland::Weak<frontend::WlSurface> const& override;
    auto session() const -> std::weak_ptr<Session> override;
private:
    std::weak_ptr<Session> const session_;
    wayland::Weak<frontend::WlSurface> const wayland_surface_;
};
}
}

#endif //MIR_SCENE_WAYLAND_BASIC_SURFACE_H
