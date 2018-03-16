/*
 * Copyright © 2015-2018 Canonical Ltd.
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
 */

#ifndef MIR_FRONTEND_WL_SURFACE_ROLE_H
#define MIR_FRONTEND_WL_SURFACE_ROLE_H

#include "mir/frontend/surface_id.h"
#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"
#include "mir/optional_value.h"

#include <memory>

struct wl_client;
struct wl_resource;

namespace mir
{
namespace scene
{
struct SurfaceCreationParameters;
}
namespace shell
{
struct SurfaceSpecification;
}
namespace frontend
{
class Shell;
class BasicSurfaceEventSink;
class WlSurface;
class WlSurfaceState;

class WlSurfaceRole
{
public:
    virtual bool synchronized() const { return false; }
    virtual void invalidate_buffer_list() = 0;
    virtual void commit(WlSurfaceState const& state) = 0;
    virtual void visiblity(bool visible) = 0;
    virtual void destroy() = 0;
    virtual ~WlSurfaceRole() = default;
} extern * const null_wl_surface_role_ptr;

class WlAbstractMirWindow : public WlSurfaceRole
{
public:
    WlAbstractMirWindow(wl_client* client, wl_resource* surface, wl_resource* event_sink,
        std::shared_ptr<frontend::Shell> const& shell);

    ~WlAbstractMirWindow() override;

    void invalidate_buffer_list() override;

protected:
    std::shared_ptr<bool> const destroyed;
    wl_client* const client;
    WlSurface* const surface;
    wl_resource* const event_sink;
    std::shared_ptr<frontend::Shell> const shell;
    std::shared_ptr<BasicSurfaceEventSink> sink;

    std::unique_ptr<scene::SurfaceCreationParameters> const params;
    SurfaceId surface_id;
    optional_value<geometry::Size> window_size_;

    geometry::Size window_size();
    shell::SurfaceSpecification& spec();

private:
    std::unique_ptr<shell::SurfaceSpecification> pending_changes;
    bool buffer_list_needs_refresh = true;

    void commit(WlSurfaceState const& state) override;
    void visiblity(bool visible) override;
};

}
}

#endif // MIR_FRONTEND_WL_SURFACE_ROLE_H
