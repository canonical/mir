/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_WL_SURFACE_ROLE_H
#define MIR_FRONTEND_WL_SURFACE_ROLE_H

#include <mir/frontend/surface_id.h>
#include <mir/geometry/displacement.h>

#include <mir_toolkit/common.h>

#include <memory>
#include <optional>

namespace mir
{
namespace scene
{
class Surface;
}
namespace frontend
{
struct WlSurfaceState;

class WlSurfaceRole
{
public:
    virtual bool synchronized() const { return false; }
    virtual auto total_offset() const -> geometry::Displacement { return {}; }
    virtual auto scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>> = 0;
    virtual void refresh_surface_data_now() = 0;
    virtual void commit(WlSurfaceState const& state) = 0;
    virtual void surface_destroyed() = 0;

    WlSurfaceRole() = default;
    virtual ~WlSurfaceRole() = default;
    WlSurfaceRole(WlSurfaceRole const&) = delete;
    WlSurfaceRole& operator=(WlSurfaceRole const&) = delete;
};
}
}

#endif // MIR_FRONTEND_WL_SURFACE_ROLE_H
