/*
 * Copyright © 2012 Canonical Ltd.
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

#include "mir/default_server_configuration.h"
#include "mir/options/option.h"
#include "mir/frontend/shell.h"
#include "mir/shell/session_container.h"
#include "mir/shell/session.h"
#include "protobuf_socket_communicator.h"
#include "mir/frontend/communicator_report.h"
#include "server_connection_implementations.h"

namespace mf = mir::frontend;
namespace msh = mir::shell;

std::shared_ptr<mf::SocketConnection> mir::DefaultServerConfiguration::the_socket()
{
    return socket([this]() -> std::shared_ptr<mf::SocketConnection>
    {
        auto const& options = the_options();
        if (options->is_set("socket-fd"))
        {
            return std::make_shared<mf::SocketPairConnection>();
        }
        else
        {
            auto const& socket_file = options->get(server_socket_opt, default_server_socket);
            auto const& result = std::make_shared<mf::FileSocketConnection>(socket_file);

            // Record this for any children that want to know how to connect to us.
            // By both listening to this env var on startup and resetting it here,
            // we make it easier to nest Mir servers.
            setenv("MIR_SOCKET", result->client_uri().c_str(), 1);

            return result;
        }
    });
}

std::shared_ptr<mf::Communicator>
mir::DefaultServerConfiguration::the_communicator()
{
    return communicator(
        [&,this]() -> std::shared_ptr<mf::Communicator>
        {
            auto const threads = the_options()->get("ipc-thread-pool", 10);
            auto shell_sessions = the_shell_session_container();
            auto const& force_requests_to_complete = [shell_sessions]
            {
                shell_sessions->for_each([](std::shared_ptr<msh::Session> const& session)
                {
                    session->force_requests_to_complete();
                });
            };

            return std::make_shared<mf::ProtobufSocketCommunicator>(
                the_socket(),
                the_ipc_factory(the_frontend_shell(), the_buffer_allocator()),
                the_session_authorizer(),
                threads,
                force_requests_to_complete,
                std::make_shared<mf::NullCommunicatorReport>());
        });
}

