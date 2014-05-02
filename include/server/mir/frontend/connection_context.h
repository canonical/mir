/*
 * Copyright © 014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_FRONTEND_CONNECTION_CONTEXT_H_
#define MIR_FRONTEND_CONNECTION_CONTEXT_H_

#include <functional>
#include <memory>

namespace mir
{
namespace frontend
{
class Connector;
class Session;

// TODO can this grow to become a real class?
struct ConnectionContext
{
    ConnectionContext(
        std::function<void(std::shared_ptr<Session> const& session)> const connect_handler,
        Connector const* connector) :
            connect_handler(connect_handler), connector(connector) {}

    ConnectionContext(Connector const* connector) :
        ConnectionContext([](std::shared_ptr<Session> const&){}, connector) {}

    ConnectionContext() = delete;

    ConnectionContext(const ConnectionContext&) = default;

    ConnectionContext& operator=(const ConnectionContext&) = default;

    std::function<void(std::shared_ptr<Session> const& session)> const connect_handler;
    Connector const* const connector;
};
}
}

#endif /* MIR_FRONTEND_CONNECTION_CONTEXT_H_ */
