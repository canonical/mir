/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_server_configuration.h"
#include "mir/emergency_cleanup.h"

#include "default_ipc_factory.h"
#include "published_socket_connector.h"
#include "session_mediator_observer_multiplexer.h"

#include "mir/frontend/connector.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/frontend/protobuf_connection_creator.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/options/configuration.h"
#include "mir/options/option.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace ms = mir::scene;

namespace
{
class StubConnector : public mf::Connector
{
public:
    void start() override
    {
    }
    void stop() override
    {
    }
    auto client_socket_fd() const -> int override
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"mirclient support not enabled (pass --enable-mirclient option?)"}));
    }
    auto client_socket_fd(std::function<void(std::shared_ptr<ms::Session> const&)> const&) const
        -> int override
    {
        BOOST_THROW_EXCEPTION((std::runtime_error{"mirclient support not enabled (pass --enable-mirclient option?)"}));
    }
    auto socket_name() const -> mir::optional_value<std::string> override
    {
        return mir::optional_value<std::string>{};
    }
};
}

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
            if (!the_options()->is_set(options::enable_mirclient_opt))
            {
                return std::make_shared<StubConnector>();
            }
            else if (the_options()->is_set(options::no_server_socket_opt))
            {
                return std::make_shared<mf::BasicConnector>(
                    the_connection_creator(),
                    the_connector_report());
            }
            else
            {
                auto const result = std::make_shared<mf::PublishedSocketConnector>(
                    the_socket_file(),
                    the_connection_creator(),
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

        bool set_base_display_configuration_is_allowed(mf::SessionCredentials const& /* creds */) override
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

        bool configure_input_is_allowed(mf::SessionCredentials const& /* creds */) override
        {
            return true;
        }

        bool set_base_input_configuration_is_allowed(mf::SessionCredentials const& /* creds */) override
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
            if (!the_options()->is_set(options::enable_mirclient_opt))
            {
                return std::make_shared<StubConnector>();
            }
            if (the_options()->is_set(options::prompt_socket_opt))
            {
                return std::make_shared<mf::PublishedSocketConnector>(
                    the_socket_file() + "_trusted",
                    the_prompt_connection_creator(),
                    *the_emergency_cleanup(),
                    the_connector_report());
            }
            else
            {
                return std::make_shared<mf::BasicConnector>(
                    the_prompt_connection_creator(),
                    the_connector_report());
            }
        });
}

std::shared_ptr<mir::frontend::ProtobufIpcFactory>
mir::DefaultServerConfiguration::new_ipc_factory(
    std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer)
{
    return std::make_shared<mf::DefaultIpcFactory>(
                the_frontend_shell(),
                the_session_mediator_observer(),
                the_graphics_platform()->make_ipc_operations(),
                the_frontend_display_changer(),
                the_buffer_allocator(),
                the_screencast(),
                session_authorizer,
                the_cursor_images(),
                the_coordinate_translator(),
                the_application_not_responding_detector(),
                the_cookie_authority(),
                the_input_configuration_changer(),
                the_extensions());
}

std::shared_ptr<mf::SessionMediatorObserver>
mir::DefaultServerConfiguration::the_session_mediator_observer()
{
    return session_mediator_observer_multiplexer(
        [default_executor = the_main_loop()]()
        {
            return std::make_shared<mf::SessionMediatorObserverMultiplexer>(default_executor);
        });
}

std::shared_ptr<mir::ObserverRegistrar<mf::SessionMediatorObserver>>
mir::DefaultServerConfiguration::the_session_mediator_observer_registrar()
{
    return session_mediator_observer_multiplexer(
        [default_executor = the_main_loop()]()
        {
            return std::make_shared<mf::SessionMediatorObserverMultiplexer>(default_executor);
        });
}
