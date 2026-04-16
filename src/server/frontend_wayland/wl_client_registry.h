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

#ifndef MIR_WL_CLIENT_REGISTRY_H
#define MIR_WL_CLIENT_REGISTRY_H

#include "wayland_rs/src/ffi.rs.h"
#include <memory>
#include <vector>

namespace mir
{
namespace frontend
{
class WlClient;

/// A global registry of the connected Wayland clients.
///
/// This interface bridges the gap between the [wayland_rs::WaylandClient] arriving
/// to us from Rust and the [WlClient] class in Mir which connects us to the shell
/// via [mir::shell::Session].
class WlClientRegistry
{
public:
    WlClientRegistry();
    virtual ~WlClientRegistry() = default;

    WlClient* add_client(rust::Box<wayland_rs::WaylandClient> client);
    void delete_client(rust::Box<wayland_rs::WaylandClientId> client);

    /// Resolve a client from its opaque identifier.
    ///
    /// This may be `nullptr` if the identifier is not found.
    WlClient* from_id(rust::Box<wayland_rs::WaylandClientId> client_id);

    /// Resolve a client from its raw client.
    ///
    /// This may be `nullptr` if the identifier is not found.
    WlClient* from_client(rust::Box<wayland_rs::WaylandClient> const& client);

private:
    WlClientRegistry(const WlClientRegistry&) = delete;
    WlClientRegistry& operator=(const WlClientRegistry&) = delete;

    std::vector<std::unique_ptr<WlClient>> clients;
};
}
}

#endif //MIR_WL_CLIENT_REGISTRY_H
