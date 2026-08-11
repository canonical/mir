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

#ifndef MIR_FRONTEND_WAYLAND_CLIENT_H_
#define MIR_FRONTEND_WAYLAND_CLIENT_H_

#include "client.h"

#include <atomic>
#include <deque>
#include <memory>
#include <utility>

struct MirEvent;

namespace mir
{
namespace scene
{
class Session;
}
namespace shell
{
class Shell;
}

namespace frontend
{
/// A monotonic serial source shared by all clients of a single Wayland display.
///
/// libwayland assigns serials from a single display-global counter
/// (`wl_display_next_serial`); we reproduce that here so serials stay unique
/// across the whole frontend.
using WaylandSerialSource = std::shared_ptr<std::atomic<uint32_t>>;

/// A concrete `mir::wayland::Client` backed by a Mir session.
///
/// This is the wayland_rs replacement for `WlClient`: it wraps the raw Rust
/// `WaylandClient`, owns the associated `scene::Session`, and tracks the
/// serial/event pairs used to correlate client requests with the input events
/// that prompted them.
///
/// Instances are created by the `WaylandClientNotifier` on client connect,
/// stored in the `wayland::WaylandClientRegistry`, and handed (as a
/// `shared_ptr<wayland::Client>`) to every Wayland object the client makes.
class WaylandClient : public wayland::Client
{
public:
    WaylandClient(
        wayland::RawWlClient raw_client,
        std::shared_ptr<scene::Session> session,
        std::shared_ptr<shell::Shell> shell,
        WaylandSerialSource serial_source);

    ~WaylandClient() override;

    auto raw_client() const -> wayland::RawWlClient const& override;
    auto is_being_destroyed() const -> bool override;
    auto client_session() const -> std::shared_ptr<scene::Session> override;
    auto next_serial(std::shared_ptr<MirEvent const> event) -> uint32_t override;
    auto event_for(uint32_t serial) -> std::optional<std::shared_ptr<MirEvent const>> override;
    void set_output_geometry_scale(float scale) override;
    auto output_geometry_scale() -> float override;

    /// Called when the Rust client's destroy listener has fired. Flags the
    /// client as being destroyed and closes the associated Mir session. The
    /// object itself may outlive this call while its resources are torn down.
    void mark_being_destroyed();

private:
    wayland::RawWlClient const raw;
    std::shared_ptr<scene::Session> const session;
    std::shared_ptr<shell::Shell> const shell;
    WaylandSerialSource const serial_source;

    bool being_destroyed{false};
    std::deque<std::pair<uint32_t, std::shared_ptr<MirEvent const>>> serial_event_pairs;
    float output_geometry_scale_{1.0f};
};
}
}

#endif // MIR_FRONTEND_WAYLAND_CLIENT_H_
