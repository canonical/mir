/*
 * Copyright © 2012-2014 Canonical Ltd.
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

#include "default_ipc_factory.h"
#include "published_socket_connector.h"

#include "mir/frontend/protobuf_connection_creator.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;

std::shared_ptr<mf::ConnectionCreator>
mir::DefaultServerConfiguration::the_connection_creator()
{
    return connection_creator([this]
        {
            return std::make_shared<mf::ProtobufConnectionCreator>(
                the_ipc_factory(the_frontend_shell(), the_buffer_allocator()),
                the_session_authorizer(),
                the_message_processor_report());
        });
}

std::shared_ptr<mf::Connector>
mir::DefaultServerConfiguration::the_connector()
{
    return connector(
        [&,this]() -> std::shared_ptr<mf::Connector>
        {
            auto const threads = the_options()->get<int>(options::frontend_threads_opt);

            if (the_options()->is_set(options::no_server_socket_opt))
            {
                return std::make_shared<mf::BasicConnector>(
                    the_connection_creator(),
                    threads,
                    the_connector_report());
            }
            else
            {
                return std::make_shared<mf::PublishedSocketConnector>(
                    the_socket_file(),
                    the_connection_creator(),
                    threads,
                    the_connector_report());
            }
        });
}

std::shared_ptr<mir::frontend::ProtobufIpcFactory>
mir::DefaultServerConfiguration::the_ipc_factory(
    std::shared_ptr<mf::Shell> const& shell,
    std::shared_ptr<mg::GraphicBufferAllocator> const& allocator)
{
    return ipc_factory(
        [&]()
        {
            return std::make_shared<mf::DefaultIpcFactory>(
                shell,
                the_session_mediator_report(),
                the_graphics_platform(),
                the_frontend_display_changer(),
                allocator,
                the_screencast(),
                the_session_authorizer());
        });
}
