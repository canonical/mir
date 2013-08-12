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

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mc = mir::compositor;
namespace msh = mir::shell;

std::shared_ptr<mf::Communicator>
mir::DefaultServerConfiguration::the_communicator()
{
    return communicator(
        [&,this]() -> std::shared_ptr<mf::Communicator>
        {
            auto const threads = the_options()->get("ipc-thread-pool", 10);
            auto shell_sessions = the_shell_session_container();
            return std::make_shared<mf::ProtobufSocketCommunicator>(
                the_socket_file(),
                the_ipc_factory(the_frontend_shell(), the_buffer_allocator()),
                the_session_authorizer(),
                threads,
                [shell_sessions]
                {
                    shell_sessions->for_each([](std::shared_ptr<msh::Session> const& session)
                    {
                        session->force_requests_to_complete();
                    });
                },
                std::make_shared<mf::NullCommunicatorReport>());
        });
}

