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

#include "wl_client.h"
#include <memory>
#include <vector>

namespace mir
{
namespace shell
{
class Shell;
}

namespace frontend
{
class WlClient;
class SessionAuthorizer;

/// A global registry of the connected Wayland clients.
///
/// This interface bridges the gap between the [wayland_rs::WaylandClient] arriving
/// to us from Rust and the [WlClient] class in Mir which connects us to the shell
/// via [mir::shell::Session].
class WlClientRegistry
{
public:
    WlClientRegistry(
        std::shared_ptr<shell::Shell> const& shell,
        std::shared_ptr<SessionAuthorizer> const& session_authorizer);
    virtual ~WlClientRegistry() = default;

    WlClient* add_client(wayland_rs::RawWlClient client);
    void delete_client(wayland_rs::RawWlClientId client);
    std::shared_ptr<WlClient> from(wayland_rs::RawWlClient const& client);
    std::shared_ptr<WlClient> from(wayland_rs::RawWlClientId const& client);

private:
    WlClientRegistry(const WlClientRegistry&) = delete;
    WlClientRegistry& operator=(const WlClientRegistry&) = delete;

    std::shared_ptr<shell::Shell> const shell;
    std::shared_ptr<SessionAuthorizer> const session_authorizer;
    std::vector<std::shared_ptr<WlClient>> clients;
};
}
}

#endif //MIR_WL_CLIENT_REGISTRY_H
