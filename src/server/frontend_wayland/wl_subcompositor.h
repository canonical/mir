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

#include "wayland.h"
#include "weak.h"
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
namespace frontend
{

class WlSurface;

class WlSubcompositor : public wayland_rs::Subcompositor
{
public:
    WlSubcompositor(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::SubcompositorMiddleware> instance,
        uint32_t object_id);

private:
    auto get_subsurface(
        wayland_rs::Weak<wayland_rs::Surface> const& surface,
        wayland_rs::Weak<wayland_rs::Surface> const& parent,
        rust::Box<wayland_rs::SubsurfaceMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::Subsurface> override;
};

class WlSubsurface: public WlSurfaceRole, public wayland_rs::Subsurface
{
public:
    WlSubsurface(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::SubsurfaceMiddleware> instance,
        uint32_t object_id,
        WlSurface* surface,
        WlSurface* parent_surface);
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
    void place_above(wayland_rs::Weak<wayland_rs::Surface> const& sibling) override;
    void place_below(wayland_rs::Weak<wayland_rs::Surface> const& sibling) override;
    void set_sync() override;
    void set_desync() override;

    void refresh_surface_data_now() override;
    virtual void commit(WlSurfaceState const& state) override;
    void surface_destroyed() override;

    WlSurface* const surface;
    /// This class is responsible for removing itself from the parent's children list when needed
    wayland_rs::Weak<WlSurface> const parent;
    bool synchronized_;
    std::optional<WlSurfaceState> cached_state;
};

}
}

#endif // MIR_FRONTEND_WL_SUBSURFACE_H
