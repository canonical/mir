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

#include "mir/frontend/protobuf_ipc_factory.h"
#include "mir/protobuf/google_protobuf_guard.h"

#include <boost/signals2.hpp>


namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;
namespace ba = boost::asio;


mf::ProtobufSocketCommunicator::ProtobufSocketCommunicator(
    std::string const& socket_file,
    std::shared_ptr<ProtobufIpcFactory> const& ipc_factory,
    int threads)
:   socket_file((std::remove(socket_file.c_str()), socket_file)),
    acceptor(io_service, socket_file),
    io_service_threads(threads),
    ipc_factory(ipc_factory),
    next_session_id(0),
    connected_sessions(std::make_shared<mfd::ConnectedSessions<mfd::SocketSession>>())
{
    start_accept();
}

void mf::ProtobufSocketCommunicator::start_accept()
{
    auto const& socket_session = std::make_shared<mfd::SocketSession>(
        io_service,
        next_id(),
        connected_sessions);

    auto session = std::make_shared<detail::ProtobufMessageProcessor>(
        socket_session.get(),
        ipc_factory->make_ipc_server(),
        ipc_factory->resource_cache(),
        ipc_factory->report());

    socket_session->set_processor(session);

    acceptor.async_accept(
        socket_session->get_socket(),
        boost::bind(
            &ProtobufSocketCommunicator::on_new_connection,
            this,
            socket_session,
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
    auto run_io_service = boost::bind(&ba::io_service::run, &io_service);

    for (auto& thread : io_service_threads)
    {
        thread = std::move(std::thread(run_io_service));
    }
}

mf::ProtobufSocketCommunicator::~ProtobufSocketCommunicator()
{
    io_service.stop();

    for (auto& thread : io_service_threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    connected_sessions->clear();

    std::remove(socket_file.c_str());
}

void mf::ProtobufSocketCommunicator::on_new_connection(
    std::shared_ptr<mfd::SocketSession> const& session,
    const boost::system::error_code& ec)
{
    if (!ec)
    {
        connected_sessions->add(session);

        session->read_next_message();
    }
    start_accept();
}
