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
    std::string const& socket_file,
    std::shared_ptr<ProtobufIpcFactory> const& ipc_factory,
    std::shared_ptr<mf::SessionAuthorizer> const& session_authorizer,
    int threads,
    std::function<void()> const& force_requests_to_complete,
    std::shared_ptr<CommunicatorReport> const& report)
:   socket_file((std::remove(socket_file.c_str()), socket_file)),
    acceptor(io_service, socket_file),
    io_service_threads(threads),
    ipc_factory(ipc_factory),
    session_authorizer(session_authorizer),
    next_session_id(0),
    connected_sessions(std::make_shared<mfd::ConnectedSessions<mfd::SocketSession>>()),
    force_requests_to_complete(force_requests_to_complete),
    report(report)
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

int mf::ProtobufSocketCommunicator::next_id()
{
    int id = next_session_id.load();
    while (!next_session_id.compare_exchange_weak(id, id + 1)) std::this_thread::yield();
    return id;
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

    connected_sessions->clear();

    std::remove(socket_file.c_str());
}

void mf::ProtobufSocketCommunicator::on_new_connection(
    std::shared_ptr<boost::asio::local::stream_protocol::socket> const& socket,
    boost::system::error_code const& ec)
{
    if (!ec)
    {
        auto messenger = std::make_shared<detail::SocketMessenger>(socket); 
        if (session_authorizer->connection_is_allowed(messenger->client_pid()))
        {
            auto event_sink = std::make_shared<detail::EventSender>(messenger);
            auto msg_processor = std::make_shared<detail::ProtobufMessageProcessor>(
                messenger,
                ipc_factory->make_ipc_server(event_sink),
                ipc_factory->resource_cache(),
                ipc_factory->report());
            auto const& session = std::make_shared<mfd::SocketSession>(
                messenger,
                next_id(),
                connected_sessions,
                msg_processor);
            connected_sessions->add(session);
            session->read_next_message();
        }
    }
    start_accept();
}

void mf::NullCommunicatorReport::error(std::exception const& /*error*/)
{
}
