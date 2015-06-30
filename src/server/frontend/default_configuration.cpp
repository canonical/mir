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
#include "mir/emergency_cleanup.h"

#include "default_ipc_factory.h"
#include "published_socket_connector.h"

#include "unsupported_coordinate_translator.h"

#include "mir/graphics/platform.h"
#include "mir/frontend/protobuf_connection_creator.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ms = mir::scene;

std::shared_ptr<mf::ConnectionCreator>
mir::DefaultServerConfiguration::the_connection_creator()
{
    return connection_creator([this]
        {
            auto const session_authorizer = the_session_authorizer();
            return std::make_shared<mf::ProtobufConnectionCreator>(
                new_ipc_factory(session_authorizer),
                session_authorizer,
                the_graphics_platform()->make_ipc_operations(),
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
                auto const result = std::make_shared<mf::PublishedSocketConnector>(
                    the_socket_file(),
                    the_connection_creator(),
                    threads,
                    *the_emergency_cleanup(),
                    the_connector_report());

                if (the_options()->is_set(options::arw_server_socket_opt))
                    chmod(the_socket_file().c_str(), S_IRUSR|S_IWUSR| S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH);

                return result;
            }
        });
}

std::shared_ptr<mf::ConnectionCreator>
mir::DefaultServerConfiguration::the_prompt_connection_creator()
{
    struct PromptSessionAuthorizer : public mf::SessionAuthorizer
    {
        bool connection_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

        bool configure_display_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

        bool screencast_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

        bool prompt_session_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }
    };

    return prompt_connection_creator([this]
        {
            auto const session_authorizer = std::make_shared<PromptSessionAuthorizer>();
            return std::make_shared<mf::ProtobufConnectionCreator>(
                new_ipc_factory(session_authorizer),
                session_authorizer,
                the_graphics_platform()->make_ipc_operations(),
                the_message_processor_report());
        });
}

std::shared_ptr<mf::Connector>
mir::DefaultServerConfiguration::the_prompt_connector()
{
    return prompt_connector(
        [&,this]() -> std::shared_ptr<mf::Connector>
        {
            auto const threads = the_options()->get<int>(options::frontend_threads_opt);

            if (the_options()->is_set(options::prompt_socket_opt))
            {
                return std::make_shared<mf::PublishedSocketConnector>(
                    the_socket_file() + "_trusted",
                    the_prompt_connection_creator(),
                    threads,
                    *the_emergency_cleanup(),
                    the_connector_report());
            }
            else
            {
                return std::make_shared<mf::BasicConnector>(
                    the_prompt_connection_creator(),
                    threads,
                    the_connector_report());
            }
        });
}

std::shared_ptr<mir::frontend::ProtobufIpcFactory>
mir::DefaultServerConfiguration::new_ipc_factory(
    std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer)
{
    std::shared_ptr<ms::CoordinateTranslator> translator;
    if (the_options()->is_set(options::debug_opt))
    {
        translator = the_coordinate_translator();
    }
    else
    {
        translator = std::make_shared<mf::UnsupportedCoordinateTranslator>();
    }
    return std::make_shared<mf::DefaultIpcFactory>(
                the_frontend_shell(),
                the_session_mediator_report(),
                the_graphics_platform()->make_ipc_operations(),
                the_frontend_display_changer(),
                the_buffer_allocator(),
                the_screencast(),
                session_authorizer,
                the_cursor_images(),
                translator,
                the_application_not_responding_detector());
}
