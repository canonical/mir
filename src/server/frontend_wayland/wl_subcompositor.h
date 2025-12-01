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

#ifndef MIR_FRONTEND_WL_SUBSURFACE_H
#define MIR_FRONTEND_WL_SUBSURFACE_H

#include "wayland_wrapper.h"
#include "wl_surface_role.h"
#include "wl_surface.h"

#include <vector>
#include <memory>

namespace mir
{
namespace shell
{
class StreamSpecification;
}
namespace wayland
{
class Client;
}
namespace frontend
{

class WlSurface;
class WlSubcompositorInstance;

class WlSubcompositor : wayland::Subcompositor::Global
{
public:
    WlSubcompositor(wl_display* display);

private:
    void bind(wl_resource* new_wl_subcompositor);
};

class WlSubsurface: public WlSurfaceRole, wayland::Subsurface
{
public:
    WlSubsurface(wl_resource* new_subsurface, WlSurface* surface, WlSurface* parent_surface);
    ~WlSubsurface();

    void populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                               std::vector<mir::geometry::Rectangle>& input_shape_accumulator,
                               geometry::Displacement const& parent_offset) const;

    auto total_offset() const -> geometry::Displacement override;
    auto synchronized() const -> bool override;
    auto scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>> override;

    void parent_has_committed();

    auto subsurface_at(geometry::Point point) -> std::optional<WlSurface*>;

    WlSurface* get_surface() const { return surface; }

private:
    void set_position(int32_t x, int32_t y) override;
    void place_above(struct wl_resource* sibling) override;
    void place_below(struct wl_resource* sibling) override;
    void set_sync() override;
    void set_desync() override;

    void refresh_surface_data_now() override;
    virtual void commit(WlSurfaceState const& state) override;
    void surface_destroyed() override;

    WlSurface* const surface;
    /// This class is responsible for removing itself from the parent's children list when needed
    wayland::Weak<WlSurface> const parent;
    bool synchronized_;
    std::optional<WlSurfaceState> cached_state;
};

}
}

#endif // MIR_FRONTEND_WL_SUBSURFACE_H
