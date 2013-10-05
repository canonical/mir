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
#include "protobuf_session_creator.h"

#include "mir/options/option.h"
#include "mir/frontend/shell.h"
#include "mir/shell/session_container.h"
#include "mir/shell/session.h"
#include "published_socket_connector.h"

namespace mf = mir::frontend;
namespace msh = mir::shell;

namespace
{
char const* const no_server_socket_opt        = "no-file";
}

std::shared_ptr<mf::SessionCreator>
mir::DefaultServerConfiguration::the_session_creator()
{
    return session_creator([this]
        {
            return std::make_shared<mf::ProtobufSessionCreator>(
                the_ipc_factory(the_frontend_shell(), the_buffer_allocator()),
                the_session_authorizer());
        });
}

std::shared_ptr<mf::Connector>
mir::DefaultServerConfiguration::the_connector()
{
    return connector(
        [&,this]() -> std::shared_ptr<mf::Connector>
        {
            auto const threads = the_options()->get("ipc-thread-pool",
                                                    default_ipc_threads);
            auto shell_sessions = the_shell_session_container();
            auto const& force_requests_to_complete = [shell_sessions]
            {
                shell_sessions->for_each([](std::shared_ptr<msh::Session> const& session)
                {
                    session->force_requests_to_complete();
                });
            };

            if (the_options()->is_set(no_server_socket_opt))
            {
                return std::make_shared<mf::BasicConnector>(
                    the_session_creator(),
                    threads,
                    force_requests_to_complete,
                    the_connector_report());
            }
            else
            {
                return std::make_shared<mf::PublishedSocketConnector>(
                    the_socket_file(),
                    the_session_creator(),
                    threads,
                    force_requests_to_complete,
                    the_connector_report());
            }
        });
}

