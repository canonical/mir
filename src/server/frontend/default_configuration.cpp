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
#include "mir/frontend/protobuf_connection_creator.h"
#include "mir/frontend/session_credentials.h"
#include "mir/frontend/session_authorizer.h"

#include "resource_cache.h"
#include "protobuf_ipc_factory.h"
#include "published_socket_connector.h"
#include "session_mediator.h"
#include "unauthorized_display_changer.h"
#include "unauthorized_screencast.h"

#include "mir/options/configuration.h"
#include "mir/options/option.h"
#include "mir/graphics/graphic_buffer_allocator.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace msh = mir::shell;

namespace
{
class DefaultIpcFactory : public mf::ProtobufIpcFactory
{
public:
    explicit DefaultIpcFactory(
        std::shared_ptr<mf::Shell> const& shell,
        std::shared_ptr<mf::SessionMediatorReport> const& sm_report,
        std::shared_ptr<mg::Platform> const& graphics_platform,
        std::shared_ptr<mf::DisplayChanger> const& display_changer,
        std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<mf::Screencast> const& screencast,
        std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer) :
        shell(shell),
        sm_report(sm_report),
        cache(std::make_shared<mf::ResourceCache>()),
        graphics_platform(graphics_platform),
        display_changer(display_changer),
        buffer_allocator(buffer_allocator),
        screencast(screencast),
        session_authorizer(session_authorizer)
    {
    }

private:
    std::shared_ptr<mf::Shell> const shell;
    std::shared_ptr<mf::SessionMediatorReport> const sm_report;
    std::shared_ptr<mf::ResourceCache> const cache;
    std::shared_ptr<mg::Platform> const graphics_platform;
    std::shared_ptr<mf::DisplayChanger> const display_changer;
    std::shared_ptr<mg::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<mf::Screencast> const screencast;
    std::shared_ptr<mf::SessionAuthorizer> const session_authorizer;

    std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server(
        mf::SessionCredentials const& creds,
        std::shared_ptr<mf::EventSink> const& sink,
        mf::ConnectionContext const& connection_context) override
    {
        std::shared_ptr<mf::DisplayChanger> changer;
        std::shared_ptr<mf::Screencast> effective_screencast;

        if (session_authorizer->configure_display_is_allowed(creds))
        {
            changer = display_changer;
        }
        else
        {
            changer = std::make_shared<mf::UnauthorizedDisplayChanger>(display_changer);
        }

        if (session_authorizer->screencast_is_allowed(creds))
        {
            effective_screencast = screencast;
        }
        else
        {
            effective_screencast = std::make_shared<mf::UnauthorizedScreencast>();
        }

        return std::make_shared<mf::SessionMediator>(
            creds.pid(),
            shell,
            graphics_platform,
            changer,
            buffer_allocator->supported_pixel_formats(),
            sm_report,
            sink,
            resource_cache(),
            effective_screencast,
            connection_context);
    }

    virtual std::shared_ptr<mf::ResourceCache> resource_cache()
    {
        return cache;
    }
};
}

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
            return std::make_shared<DefaultIpcFactory>(
                shell,
                the_session_mediator_report(),
                the_graphics_platform(),
                the_frontend_display_changer(),
                allocator,
                the_screencast(),
                the_session_authorizer());
        });
}
