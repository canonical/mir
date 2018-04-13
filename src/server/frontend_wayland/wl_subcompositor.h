/*
 * Copyright © 2018 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 *              William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_WL_SUBSURFACE_H
#define MIR_FRONTEND_WL_SUBSURFACE_H

#include "generated/wayland_wrapper.h"
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

class WlSubcompositor: wayland::Subcompositor
{
public:
    WlSubcompositor(struct wl_display* display);

private:
    void destroy(struct wl_client* client, struct wl_resource* resource) override;
    void get_subsurface(struct wl_client* client, struct wl_resource* resource, uint32_t id,
                        struct wl_resource* surface, struct wl_resource* parent) override;
};

class WlSubsurface: public WlSurfaceRole, wayland::Subsurface
{
public:
    WlSubsurface(struct wl_client* client, struct wl_resource* object_parent, uint32_t id, WlSurface* surface,
                 WlSurface* parent_surface);
    ~WlSubsurface();

    void populate_buffer_list(std::vector<shell::StreamSpecification>& buffers,
                              geometry::Displacement const& parent_offset) const;

    bool synchronized() const override;

    SurfaceId surface_id() const override;

    void parent_has_committed();

private:
    void set_position(int32_t x, int32_t y) override;
    void place_above(struct wl_resource* sibling) override;
    void place_below(struct wl_resource* sibling) override;
    void set_sync() override;
    void set_desync() override;

    void destroy() override; // overrides function in both WlSurfaceRole and wayland::Subsurface

    void invalidate_buffer_list() override;
    virtual void commit(WlSurfaceState const& state) override;
    virtual void visiblity(bool visible) override;

    WlSurface* const surface;
    // manages parent/child relationship, but does not manage parent's memory
    // see WlSurface::add_child() for details
    std::unique_ptr<WlSurface, std::function<void(WlSurface*)>> const parent;
    std::shared_ptr<bool> const parent_destroyed;
    bool synchronized_;
    std::experimental::optional<WlSurfaceState> cached_state;
};

}
}

#endif // MIR_FRONTEND_WL_SUBSURFACE_H
