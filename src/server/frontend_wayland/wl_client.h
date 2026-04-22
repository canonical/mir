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

#ifndef MIR_FRONTEND_WL_CLIENT_H_
#define MIR_FRONTEND_WL_CLIENT_H_

struct wl_listener;
struct wl_display;

#include <memory>
#include <functional>
#include <optional>
#include <deque>
#include "wayland_rs/wayland_rs_cpp/include/wayland.h"
#include "wayland_rs/src/ffi.rs.h"

#include <mir/wayland/client.h>

struct MirEvent;

namespace mir
{
namespace wayland_rs
{
class WaylandServer;
}
namespace shell
{
class Shell;
}

namespace frontend
{
class SessionAuthorizer;

class WlClient : public wayland::Client
{
public:
    WlClient(rust::Box<wayland_rs::WaylandClient> client, std::shared_ptr<scene::Session> const& session, shell::Shell* shell);
    ~WlClient() override;

    auto is_being_destroyed() const -> bool override { return !owned_self; }

    auto client_session() const -> std::shared_ptr<scene::Session> override { return session; }

    auto next_serial(std::shared_ptr<MirEvent const> event) -> uint32_t override;

    auto event_for(uint32_t serial) -> std::optional<std::shared_ptr<MirEvent const>> override;

    void set_output_geometry_scale(float scale) override { output_geometry_scale_ = scale; }
    auto output_geometry_scale() -> float override { return output_geometry_scale_; }

private:
    /// This shell is owned by the ClientSessionConstructor, which outlives all clients.
    shell::Shell* const shell;
    rust::Box<wayland_rs::WaylandClient> client;
    std::shared_ptr<scene::Session> const session;

    std::shared_ptr<WlClient> owned_self;
    std::deque<std::pair<uint32_t, std::shared_ptr<MirEvent const>>> serial_event_pairs;
    float output_geometry_scale_{1};
};
}
}

#endif // MIR_FRONTEND_WL_CLIENT_H_
