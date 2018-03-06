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

#ifndef MIR_FRONTEND_WL_MIR_WINDOW_H
#define MIR_FRONTEND_WL_MIR_WINDOW_H

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
class SurfaceCreationParameters;
}
namespace shell
{
class SurfaceSpecification;
}
namespace frontend
{
class Shell;
class BasicSurfaceEventSink;

class WlMirWindow
{
public:
    virtual void new_buffer_size(geometry::Size const& buffer_size) = 0;
    virtual void commit() = 0;
    virtual void visiblity(bool visible) = 0;
    virtual void destroy() = 0;
    virtual ~WlMirWindow() = default;
} extern * const null_wl_mir_window_ptr;

class WlAbstractMirWindow : public WlMirWindow
{
public:
    WlAbstractMirWindow(wl_client* client, wl_resource* surface, wl_resource* event_sink,
                        std::shared_ptr<frontend::Shell> const& shell);

    ~WlAbstractMirWindow() override;

    std::shared_ptr<bool> const destroyed;
    std::shared_ptr<BasicSurfaceEventSink> sink;

protected:
    wl_client* const client;
    wl_resource* const surface;
    wl_resource* const event_sink;
    std::shared_ptr<frontend::Shell> const shell;

    std::unique_ptr<scene::SurfaceCreationParameters> const params;
    SurfaceId surface_id;
    optional_value<geometry::Size> window_size;

    shell::SurfaceSpecification& spec();

private:
    geometry::Size latest_buffer_size;
    std::unique_ptr<shell::SurfaceSpecification> pending_changes;

    void commit() override;
    void new_buffer_size(geometry::Size const& buffer_size) override;
    void visiblity(bool visible) override;
};

}
}

#endif // MIR_FRONTEND_WL_MIR_WINDOW_H
