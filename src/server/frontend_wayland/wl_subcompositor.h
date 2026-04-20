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
#include "wayland_rs/wayland_rs_cpp/include/wayland.h"

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

class WlSubcompositor : public wayland_rs::WlSubcompositorImpl
{
public:
    explicit WlSubcompositor(rust::Box<wayland_rs::WaylandClient> client);
    auto get_subsurface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface, wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& parent)
       -> std::shared_ptr<wayland_rs::WlSubsurfaceImpl> override;
};

class WlSubsurface: public WlSurfaceRole, public wayland_rs::WlSubsurfaceImpl
{
public:
    WlSubsurface(
        rust::Box<wayland_rs::WaylandClient> client,
        wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface,
        wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& parent);
    ~WlSubsurface();

    void populate_surface_data(std::vector<shell::StreamSpecification>& buffer_streams,
                               std::vector<mir::geometry::Rectangle>& input_shape_accumulator,
                               geometry::Displacement const& parent_offset) const;

    auto total_offset() const -> geometry::Displacement override;
    auto synchronized() const -> bool override;
    auto scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>> override;

    void parent_has_committed();

    auto subsurface_at(geometry::Point point) -> std::optional<WlSurface*>;

    wayland_rs::Weak<wayland_rs::WlSurfaceImpl> get_surface() const { return surface; }

private:
    void place_above(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& sibling) override;
    void place_below(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& sibling) override;
    void set_position(int32_t x, int32_t y) override;
    void set_sync() override;
    void set_desync() override;

    void refresh_surface_data_now() override;
    virtual void commit(WlSurfaceState const& state) override;
    void surface_destroyed() override;

    wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const surface;
    wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const parent;
    bool synchronized_;
    std::optional<WlSurfaceState> cached_state;
};

}
}

#endif // MIR_FRONTEND_WL_SUBSURFACE_H
