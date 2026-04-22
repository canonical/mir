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

#include "wl_client.h"
#include "std_layout_uptr.h"
#include "wayland_rs/src/ffi.rs.h"

#include <mir/frontend/session_authorizer.h>
#include <mir/frontend/session_credentials.h>
#include <mir/shell/shell.h>
#include <mir/scene/session.h>
#include <mir/fatal.h>

#include <wayland-server-core.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;

mf::WlClient::WlClient(RawWlClient client, std::shared_ptr<ms::Session> const& session, msh::Shell* shell)
    : shell{shell},
      client{std::move(client)},
      session{session},
      owned_self(std::make_shared<int>(0))
{
}

mf::WlClient::~WlClient()
{
    shell->close_session(session);
}

auto mf::WlClient::next_serial(std::shared_ptr<MirEvent const> event) -> uint32_t
{
    constexpr int max_serial_event_pairs = 100;
    auto const serial = wayland_rs::next_serial();
    serial_event_pairs.push_front({serial, event});
    while (serial_event_pairs.size() > max_serial_event_pairs)
    {
        serial_event_pairs.pop_back();
    }
    return serial;
}

auto mf::WlClient::event_for(uint32_t serial) -> std::optional<std::shared_ptr<MirEvent const>>
{
    for (auto const& pair : serial_event_pairs)
    {
        if (pair.first == serial)
        {
            return pair.second;
        }
    }
    return std::nullopt;
}
