/*
 * Copyright © 2020 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_XWAYLAND_SURFACE_ROLE_H
#define MIR_FRONTEND_XWAYLAND_SURFACE_ROLE_H

#include "wl_surface_role.h"

#include <mutex>

namespace mir
{
namespace scene
{
class Surface;
}
namespace shell
{
class Shell;
}
namespace frontend
{
class WlSurface;
class XWaylandSurfaceRoleSurface;

/// Should only be created on the Wayland thread
/// Destroys itself when the wl_surface is destroyed
class XWaylandSurfaceRole : private WlSurfaceRole
{
public:
    XWaylandSurfaceRole(
        std::shared_ptr<shell::Shell> const& shell,
        std::shared_ptr<XWaylandSurfaceRoleSurface> const& wm_surface,
        WlSurface* wl_surface);
    ~XWaylandSurfaceRole(); ///< Must be called on the Wayland thread!

private:
    std::shared_ptr<shell::Shell> const shell;
    std::weak_ptr<XWaylandSurfaceRoleSurface> const weak_wm_surface;
    WlSurface* const wl_surface;

    /// Overrides from WlSurfaceRole
    /// @{
    auto scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>> override;
    void refresh_surface_data_now() override;
    void commit(WlSurfaceState const& state) override;
    void visiblity(bool visible) override;
    void destroy() override;
    /// @}
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_SURFACE_ROLE_H
