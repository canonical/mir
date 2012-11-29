/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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
#include "mir/frontend/application_mediator.h"
#include "mir/frontend/protobuf_ipc_factory.h"
#include "mir/frontend/resource_cache.h"
#include "mir/thread/all.h"

#include "mir_protobuf.pb.h"
#include "mir_protobuf_wire.pb.h"

#include "ancillary.h"

#include <google/protobuf/descriptor.h>

#include <map>
#include <exception>
#include <iostream>

namespace
{
// Too clever? The idea is to ensure protbuf version is verified once (on
// the first google_protobuf_guard() call) and memory is released on exit.
struct google_protobuf_guard_t
{
    google_protobuf_guard_t() { GOOGLE_PROTOBUF_VERIFY_VERSION; }
    ~google_protobuf_guard_t() { google::protobuf::ShutdownProtobufLibrary(); }
};

void google_protobuf_guard()
{
    static google_protobuf_guard_t guard;
}
bool force_init{(google_protobuf_guard(), true)};
}


namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;
namespace ba = boost::asio;
namespace bs = boost::system;


struct mfd::SocketSession : public mfd::Sender
{
    SocketSession(
        boost::asio::io_service& io_service,
        int id_,
        ConnectedSessions<SocketSession>* connected_sessions) :
        socket(io_service),
        id_(id_),
        connected_sessions(connected_sessions),
        processor(std::make_shared<NullMessageProcessor>()) {}

    int id() const { return id_; }

    void read_next_message();

    void set_processor(std::shared_ptr<MessageProcessor> const& processor)
    {
        this->processor = processor;
    }

    boost::asio::local::stream_protocol::socket&
    get_socket() { return socket; }

private:
    void send(const std::ostringstream& buffer2);
    void send_fds(std::vector<int32_t> const& fd)
    {
        if (fd.size() > 0)
        {
            ancil_send_fds(socket.native_handle(), fd.data(), fd.size());
        }
    }

    void on_response_sent(boost::system::error_code const& error, std::size_t);
    void on_new_message(const boost::system::error_code& ec);
    void on_read_size(const boost::system::error_code& ec);

    boost::asio::local::stream_protocol::socket socket;
    int const id_;
    ConnectedSessions<SocketSession>* connected_sessions;
    std::shared_ptr<MessageProcessor> processor;
    boost::asio::streambuf message;
    unsigned char message_header_bytes[2];
    std::vector<char> whole_message;
};

mf::ProtobufSocketCommunicator::ProtobufSocketCommunicator(
    std::string const& socket_file,
    std::shared_ptr<ProtobufIpcFactory> const& ipc_factory)
:   socket_file((std::remove(socket_file.c_str()), socket_file)),
    acceptor(io_service, socket_file),
    ipc_factory(ipc_factory),
    next_session_id(0)
{
    start_accept();
}

void mf::ProtobufSocketCommunicator::start_accept()
{
    auto const& socket_session = std::make_shared<mfd::SocketSession>(
        io_service,
        next_id(),
        &connected_sessions);

    auto session = std::make_shared<detail::ProtobufMessageProcessor>(
        socket_session.get(),
        ipc_factory->make_ipc_server(),
        ipc_factory->resource_cache());

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
    io_service_thread = std::move(std::thread(run_io_service));
}

mf::ProtobufSocketCommunicator::~ProtobufSocketCommunicator()
{
    io_service.stop();

    if (io_service_thread.joinable())
    {
        io_service_thread.join();
    }

    connected_sessions.clear();

    std::remove(socket_file.c_str());
}

void mf::ProtobufSocketCommunicator::on_new_connection(
    std::shared_ptr<mfd::SocketSession> const& session,
    const boost::system::error_code& ec)
{
    if (!ec)
    {
        connected_sessions.add(session);

        session->read_next_message();
    }
    start_accept();
}

void mfd::SocketSession::read_next_message()
{
    boost::asio::async_read(socket,
        boost::asio::buffer(message_header_bytes),
        boost::bind(&mfd::SocketSession::on_read_size,
            this, ba::placeholders::error));
}

void mfd::SocketSession::on_read_size(const boost::system::error_code& ec)
{
    if (!ec)
    {
        size_t const body_size = (message_header_bytes[0] << 8) + message_header_bytes[1];
        // Read newline delimited messages for now
        ba::async_read(
             socket,
             message,
             boost::asio::transfer_exactly(body_size),
             boost::bind(&mfd::SocketSession::on_new_message,
                         this, ba::placeholders::error));
    }
}

void mfd::SocketSession::on_new_message(const boost::system::error_code& ec)
{
    bool alive{true};

    if (!ec)
    {
        std::istream msg(&message);
        alive = processor->process_message(msg);
    }

    if (alive) read_next_message();
    else connected_sessions->remove(id());
}

void mfd::SocketSession::on_response_sent(bs::error_code const& error, std::size_t)
{
    if (error)
        std::cerr << "ERROR sending response: " << error.message() << std::endl;
}

void mfd::SocketSession::send(const std::ostringstream& buffer2)
{
    const std::string& body = buffer2.str();
    const size_t size = body.size();
    const unsigned char header_bytes[2] =
    { static_cast<unsigned char>((size >> 8) & 0xff), static_cast<unsigned char>((size >> 0) & 0xff) };
    whole_message.resize(sizeof header_bytes + size);
    std::copy(header_bytes, header_bytes + sizeof header_bytes, whole_message.begin());
    std::copy(body.begin(), body.end(), whole_message.begin() + sizeof header_bytes);
    ba::async_write(socket, ba::buffer(whole_message),
        boost::bind(&mfd::SocketSession::on_response_sent, this, boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}
