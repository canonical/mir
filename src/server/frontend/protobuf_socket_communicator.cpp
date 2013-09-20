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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#include "protobuf_socket_communicator.h"
#include "protobuf_message_processor.h"
#include "socket_session.h"

#include "mir/frontend/communicator_report.h"
#include "mir/frontend/protobuf_ipc_factory.h"
#include "mir/frontend/session_authorizer.h"
#include "socket_messenger.h"
#include "event_sender.h"
#include "mir/protobuf/google_protobuf_guard.h"

#include <boost/signals2.hpp>

namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;
namespace ba = boost::asio;

mf::ProtobufSocketCommunicator::ProtobufSocketCommunicator(
    std::shared_ptr<SocketConnection> const& socket_connection,
    std::shared_ptr<ProtobufIpcFactory> const& ipc_factory,
    std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer,
    int threads,
    std::function<void()> const& force_requests_to_complete,
    std::shared_ptr<CommunicatorReport> const& report)
:   socket_connection(socket_connection),
    acceptor(socket_connection->acceptor(io_service)),
    io_service_threads(threads),
    force_requests_to_complete(force_requests_to_complete),
    report(report),
    session_creator{std::make_shared<ProtobufSessionCreator>(ipc_factory, session_authorizer)}
{
    start_accept();
}

void mf::ProtobufSocketCommunicator::start_accept()
{
    auto socket = std::make_shared<boost::asio::local::stream_protocol::socket>(io_service);

    acceptor.async_accept(
        *socket,
        boost::bind(
            &ProtobufSocketCommunicator::on_new_connection,
            this,
            socket,
            ba::placeholders::error));
}

void mf::ProtobufSocketCommunicator::start()
{
    auto run_io_service = [&]
    {
        while (true)
        try
        {
            io_service.run();
            return;
        }
        catch (std::exception const& e)
        {
            report->error(e);
        }
    };

    for (auto& thread : io_service_threads)
    {
        thread = std::move(std::thread(run_io_service));
    }
}

void mf::ProtobufSocketCommunicator::stop()
{
    /* Stop processing new requests */
    io_service.stop();

    /* 
     * Ensure that any pending requests will complete (i.e., that they
     * will not block indefinitely waiting for a resource from the server)
     */
    force_requests_to_complete();

    /* Wait for all io processing threads to finish */
    for (auto& thread : io_service_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    /* Prepare for a potential restart */
    io_service.reset();
}

mf::ProtobufSocketCommunicator::~ProtobufSocketCommunicator()
{
    stop();
}

void mf::ProtobufSocketCommunicator::on_new_connection(
    std::shared_ptr<boost::asio::local::stream_protocol::socket> const& socket,
    boost::system::error_code const& ec)
{
    if (!ec)
    {
        session_creator->create_session_for(socket);
    }
    start_accept();
}

mf::ProtobufSessionCreator::ProtobufSessionCreator(
    std::shared_ptr<ProtobufIpcFactory> const& ipc_factory,
    std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer)
:   ipc_factory(ipc_factory),
    session_authorizer(session_authorizer),
    next_session_id(0),
    connected_sessions(std::make_shared<mfd::ConnectedSessions<mfd::SocketSession>>())
{
}

mf::ProtobufSessionCreator::~ProtobufSessionCreator()
{
    connected_sessions->clear();
}

int mf::ProtobufSessionCreator::next_id()
{
    return next_session_id.fetch_add(1);
}

void mf::ProtobufSessionCreator::create_session_for(std::shared_ptr<boost::asio::local::stream_protocol::socket> const& socket)
{
    auto const messenger = std::make_shared<detail::SocketMessenger>(socket);
    auto const client_pid = messenger->client_pid();

    if (session_authorizer->connection_is_allowed(client_pid))
    {
        auto const authorized_to_resize_display = session_authorizer->configure_display_is_allowed(client_pid);
        auto const event_sink = std::make_shared<detail::EventSender>(messenger);
        auto const msg_processor = std::make_shared<detail::ProtobufMessageProcessor>(
            messenger,
            ipc_factory->make_ipc_server(event_sink, authorized_to_resize_display),
            ipc_factory->resource_cache(),
            ipc_factory->report());

        const auto& session = std::make_shared<mfd::SocketSession>(messenger, next_id(), connected_sessions, msg_processor);
        connected_sessions->add(session);
        session->read_next_message();
    }
}

void mf::NullCommunicatorReport::error(std::exception const& /*error*/)
{
}
