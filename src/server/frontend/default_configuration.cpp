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

#include "resource_cache.h"
#include "global_event_sender.h"
#include "protobuf_ipc_factory.h"
#include "protobuf_session_creator.h"
#include "published_socket_connector.h"
#include "session_mediator.h"

#include "mir/options/option.h"
#include "mir/shell/session_container.h"
#include "mir/shell/session.h"

// TODO this looks like a missing factory function
#include "mir/shell/unauthorized_display_changer.h"

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace msh = mir::shell;

namespace
{
char const* const no_server_socket_opt        = "no-file";

class DefaultIpcFactory : public mf::ProtobufIpcFactory
{
public:
    explicit DefaultIpcFactory(
        std::shared_ptr<mf::Shell> const& shell,
        std::shared_ptr<mf::SessionMediatorReport> const& sm_report,
        std::shared_ptr<mf::MessageProcessorReport> const& mr_report,
        std::shared_ptr<mg::Platform> const& graphics_platform,
        std::shared_ptr<mf::DisplayChanger> const& display_changer,
        std::shared_ptr<mg::GraphicBufferAllocator> const& buffer_allocator) :
        shell(shell),
        sm_report(sm_report),
        mp_report(mr_report),
        cache(std::make_shared<mf::ResourceCache>()),
        graphics_platform(graphics_platform),
        display_changer(display_changer),
        buffer_allocator(buffer_allocator)
    {
    }

private:
    std::shared_ptr<mf::Shell> shell;
    std::shared_ptr<mf::SessionMediatorReport> const sm_report;
    std::shared_ptr<mf::MessageProcessorReport> const mp_report;
    std::shared_ptr<mf::ResourceCache> const cache;
    std::shared_ptr<mg::Platform> const graphics_platform;
    std::shared_ptr<mf::DisplayChanger> const display_changer;
    std::shared_ptr<mg::GraphicBufferAllocator> const buffer_allocator;

    virtual std::shared_ptr<mir::protobuf::DisplayServer> make_ipc_server(
        std::shared_ptr<mf::EventSink> const& sink, bool authorized_to_resize_display)
    {
        std::shared_ptr<mf::DisplayChanger> changer;
        if(authorized_to_resize_display)
        {
            changer = display_changer;
        }
        else
        {
            changer = std::make_shared<msh::UnauthorizedDisplayChanger>(display_changer);
        }

        return std::make_shared<mf::SessionMediator>(
            shell,
            graphics_platform,
            changer,
            buffer_allocator,
            sm_report,
            sink,
            resource_cache());
    }

    virtual std::shared_ptr<mf::ResourceCache> resource_cache()
    {
        return cache;
    }

    virtual std::shared_ptr<mf::MessageProcessorReport> report()
    {
        return mp_report;
    }
};
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
            auto const threads = the_options()->get(frontend_threads,
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

std::shared_ptr<mf::EventSink>
mir::DefaultServerConfiguration::the_global_event_sink()
{
    return global_event_sink(
        [this]()
        {
            return std::make_shared<mf::GlobalEventSender>(the_shell_session_container());
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
                the_message_processor_report(),
                the_graphics_platform(),
                the_frontend_display_changer(), allocator);
        });
}
