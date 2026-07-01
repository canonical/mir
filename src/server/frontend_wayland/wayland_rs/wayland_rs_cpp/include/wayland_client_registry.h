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

#ifndef MIR_WAYLANDRS_WAYLAND_CLIENT_REGISTRY
#define MIR_WAYLANDRS_WAYLAND_CLIENT_REGISTRY

#include "client.h"

#include <memory>
#include <vector>

namespace mir
{
namespace wayland_rs
{
/// A registry of connected Wayland clients, mapping the per-connection raw Rust
/// client to a single shared `Client`.
///
/// Populate it from the `WaylandServerNotificationHandler` (add on connect,
/// remove on disconnect); when a Wayland object is created its raw client is
/// resolved to the corresponding `Client` via `from()`, and the resulting
/// `std::shared_ptr<Client>` is handed to the object's constructor.
///
/// This registry is deliberately generic: it stores and resolves abstract
/// `Client`s but does not know how to *create* one. Concrete `Client` creation
/// (e.g. opening a shell session) is the responsibility of the caller, which
/// builds the `Client` and hands it to `add_client()`.
///
/// This class is not synchronized: all calls must happen on the Wayland
/// event-loop thread.
class WaylandClientRegistry
{
public:
    WaylandClientRegistry() = default;
    virtual ~WaylandClientRegistry() = default;

    WaylandClientRegistry(WaylandClientRegistry const&) = delete;
    WaylandClientRegistry& operator=(WaylandClientRegistry const&) = delete;

    /// Store an already-created `Client` so it can later be resolved. If a
    /// client with the same id is already registered it is replaced.
    void add_client(std::shared_ptr<Client> const& client);

    /// Remove the `Client` matching the given id (e.g. on client disconnect).
    void remove_client(RawWlClientId const& id);

    /// Resolve the `Client` for the given raw client, or nullptr if unknown.
    auto from(RawWlClient const& client) const -> std::shared_ptr<Client>;

    /// Resolve the `Client` for the given raw client id, or nullptr if unknown.
    auto from(RawWlClientId const& id) const -> std::shared_ptr<Client>;

private:
    std::vector<std::shared_ptr<Client>> clients;
};
}
}

#endif // MIR_WAYLANDRS_WAYLAND_CLIENT_REGISTRY
