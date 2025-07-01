/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_FRONTEND_WAYLAND_H
#define MIR_FRONTEND_WAYLAND_H

#include <mir/graphics/display_configuration.h>

#include <memory>
#include <string>
#include <vector>

struct wl_client;
struct wl_resource;
namespace mir::wayland { class Client; }

namespace mir
{
namespace scene
{
class Session;
class Surface;
}
namespace frontend
{

/// Utility function to recover the session associated with a wl_client
auto get_session(wl_client* client) -> std::shared_ptr<scene::Session>;

/// Utility function to recover the session associated with a wl_resource
auto get_session(wl_resource* resource) -> std::shared_ptr<scene::Session>;

/// Utility function to recover the window associated with a wl_client
auto get_window(wl_resource* surface) -> std::shared_ptr<scene::Surface>;

/// Returns the "standard" extensions Mir recomends enabling (a subset of supported extensions)
auto get_standard_extensions() -> std::vector<std::string>;

/// Returns all extensions core Mir supports
auto get_supported_extensions() -> std::vector<std::string>;

class WaylandTools
{
public:
    WaylandTools() = default;
    virtual ~WaylandTools() = default;
    WaylandTools(WaylandTools const&) = delete;
    WaylandTools& operator=(WaylandTools const&) = delete;

    virtual void for_each_output_binding(
        wayland::Client* client,
        graphics::DisplayConfigurationOutputId output,
        std::function<void(struct wl_resource* output)> const& callback) = 0;

};
}
}

#endif //MIR_FRONTEND_WAYLAND_H
