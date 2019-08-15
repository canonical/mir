/*
 * Copyright © 2014 Canonical Ltd.
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
 *
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/connection_context.h"

#include "mir/frontend/connector.h"

namespace mf = mir::frontend;

mf::ConnectionContext::ConnectionContext(
    std::function<void(std::shared_ptr<scene::Session> const& session)> const connect_handler,
    Connector const* connector) :
    connect_handler(connect_handler),
    connector(connector)
{
}

int mf::ConnectionContext::fd_for_new_client(std::function<void(std::shared_ptr<scene::Session> const& session)> const& connect_handler) const
{
    return connector->client_socket_fd(connect_handler);
}
