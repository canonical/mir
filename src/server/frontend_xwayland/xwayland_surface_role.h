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

namespace mir
{
namespace scene
{
class Surface;
}
namespace shell
{
class Shell;
struct SurfaceSpecification;
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
        WlSurface* wl_surface,
        float scale);
    ~XWaylandSurfaceRole(); ///< Must be called on the Wayland thread!

    /// Populates the buffer streams and input shape from the surface into the spec. Scales both as needed. If scale is
    /// not 1.0 it wraps the surface's buffer streams in scaled buffer streams. These are stored in the spec as
    /// weak_ptrs, so they are also pushed onto keep_alive_until_spec_is_used. Once you call modify_surface()
    /// or otherwise use the spec, shared_ptrs will have been created and this can be dropped.
    static void populate_surface_data_scaled(
        WlSurface* wl_surface,
        float scale,
        shell::SurfaceSpecification& spec,
        std::vector<std::shared_ptr<void>>& keep_alive_until_spec_is_used);

private:
    std::shared_ptr<shell::Shell> const shell;
    std::weak_ptr<XWaylandSurfaceRoleSurface> const weak_wm_surface;
    WlSurface* const wl_surface;
    float const scale;

    /// Overrides from WlSurfaceRole
    /// @{
    auto scene_surface() const -> std::experimental::optional<std::shared_ptr<scene::Surface>> override;
    void refresh_surface_data_now() override;
    void commit(WlSurfaceState const& state) override;
    void surface_destroyed() override;
    /// @}
};
}
}

#endif // MIR_FRONTEND_XWAYLAND_SURFACE_ROLE_H
