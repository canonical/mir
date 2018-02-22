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

#include "wl_mir_window.h"

#include "wayland_connector.h"
#include "wl_surface.h"
#include "basic_surface_event_sink.h"

#include "mir/frontend/shell.h"
#include "mir/frontend/session.h"
#include "mir/frontend/event_sink.h"

#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"

namespace mir
{
namespace frontend
{

namespace
{
class NullWlMirWindow : public WlMirWindow
{
public:
    void new_buffer_size(geometry::Size const& /*buffer_size*/) {}
    void commit() {}
    void visiblity(bool /*visible*/) {}
    void destroy() {}
} null_wl_mir_window_instance;
}

WlMirWindow* const null_wl_mir_window_ptr = &null_wl_mir_window_instance;

namespace
{
std::shared_ptr<scene::Surface> get_surface_for_id(std::shared_ptr<Session> const& session, SurfaceId surface_id)
{
    return std::dynamic_pointer_cast<scene::Surface>(session->get_surface(surface_id));
}
}

WlAbstractMirWindow::WlAbstractMirWindow(wl_client* client, wl_resource* surface, wl_resource* event_sink,
    std::shared_ptr<Shell> const& shell)
        : destroyed{std::make_shared<bool>(false)},
          client{client},
          surface{surface},
          event_sink{event_sink},
          shell{shell},
          params{std::make_unique<scene::SurfaceCreationParameters>(
              scene::SurfaceCreationParameters().of_type(mir_window_type_freestyle))}
{
}

WlAbstractMirWindow::~WlAbstractMirWindow()
{
    *destroyed = true;
    if (surface_id.as_value())
    {
        if (auto session = session_for_client(client))
        {
            shell->destroy_surface(session, surface_id);
        }

        surface_id = {};
    }
}

shell::SurfaceSpecification& WlAbstractMirWindow::spec()
{
    if (!pending_changes)
        pending_changes = std::make_unique<shell::SurfaceSpecification>();

    return *pending_changes;
}

void WlAbstractMirWindow::commit()
{
    auto const session = session_for_client(client);

    if (surface_id.as_value())
    {
        auto const surface = get_surface_for_id(session, surface_id);

        if (surface->size() != latest_buffer_size)
        {
            sink->latest_resize(latest_buffer_size);
            auto& new_size_spec = spec();
            new_size_spec.width = latest_buffer_size.width;
            new_size_spec.height = latest_buffer_size.height;
        }

        if (pending_changes)
            shell->modify_surface(session, surface_id, *pending_changes);

        pending_changes.reset();
        return;
    }

    // Until we've seen a buffer we don't create a surface
    if (latest_buffer_size == geometry::Size{})
        return;

    auto* const mir_surface = WlSurface::from(surface);
    if (params->size == geometry::Size{})
        params->size = latest_buffer_size;
    params->content_id = mir_surface->stream_id;
    surface_id = shell->create_surface(session, *params, sink);
    mir_surface->surface_id = surface_id;

    // The shell isn't guaranteed to respect the requested size
    auto const window = session->get_surface(surface_id);
    sink->send_resize(window->client_size());
}

void WlAbstractMirWindow::new_buffer_size(geometry::Size const& buffer_size)
{
    latest_buffer_size = buffer_size;

    // Sometimes, when using xdg-shell, qterminal creates an insanely tall buffer
    if (latest_buffer_size.height > geometry::Height{10000})
    {
        log_warning("Insane buffer height sanitized: latest_buffer_size.height = %d (was %d)",
                1000, latest_buffer_size.height.as_int());
        latest_buffer_size.height = geometry::Height{1000};
    }
}

void WlAbstractMirWindow::visiblity(bool visible)
{
    if (!surface_id.as_value())
        return;

    auto const session = session_for_client(client);

    if (get_surface_for_id(session, surface_id)->visible() == visible)
        return;

    spec().state = visible ? mir_window_state_restored : mir_window_state_hidden;
}

}
}
