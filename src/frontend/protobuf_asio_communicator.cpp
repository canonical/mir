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

#include "mir/frontend/protobuf_asio_communicator.h"
#include "mir/thread/all.h"

#include "mir_protobuf.pb.h"
#include "mir_protobuf_wire.pb.h"

#include <google/protobuf/descriptor.h>

#include <map>

namespace mf = mir::frontend;
namespace mfd = mir::frontend::detail;
namespace ba = boost::asio;
namespace bs = boost::system;

class mfd::Session
{
public:
    Session(
        boost::asio::io_service& io_service,
        int id_,
        ConnectedSessions* connected_sessions,
        std::shared_ptr<protobuf::DisplayServer> const&);

    int id() const
    {
        return id_;
    }
private:
    void read_next_message();
    void on_response_sent(boost::system::error_code const& error, std::size_t);
    void send_response(::google::protobuf::uint32 id, google::protobuf::Message* response);
    void on_new_message(const boost::system::error_code& ec);
    void on_read_size(const boost::system::error_code& ec);

    friend class ::mir::frontend::ProtobufAsioCommunicator;

    boost::asio::local::stream_protocol::socket socket;
    boost::asio::streambuf message;
    int const id_;
    ConnectedSessions* connected_sessions;
    std::shared_ptr<protobuf::DisplayServer> const display_server;
    mir::protobuf::Surface surface;
    unsigned char message_header_bytes[2];
    std::vector<char> whole_message;
};


// TODO: Switch to std::bind for launching the thread.
mf::ProtobufAsioCommunicator::ProtobufAsioCommunicator(
    std::string const& socket_file,
    std::shared_ptr<ProtobufIpcFactory> const& ipc_factory)
:   socket_file((std::remove(socket_file.c_str()), socket_file)),
    acceptor(io_service, socket_file),
    ipc_factory(ipc_factory),
    next_session_id(0)
{
    start_accept();
}

void mf::ProtobufAsioCommunicator::start_accept()
{
    auto session = std::make_shared<detail::Session>(
        io_service,
        next_id(),
        &connected_sessions,
        ipc_factory->make_ipc_server());

    acceptor.async_accept(
        session->socket,
        boost::bind(
            &ProtobufAsioCommunicator::on_new_connection,
            this,
            session,
            ba::placeholders::error));
}

int mf::ProtobufAsioCommunicator::next_id()
{
    int id;
    do { id = next_session_id.load(); }
    while (!next_session_id.compare_exchange_weak(id, id + 1));
    return id;
}


void mf::ProtobufAsioCommunicator::start()
{
    auto run_io_service = boost::bind(&ba::io_service::run, &io_service);
    io_service_thread = std::move(std::thread(run_io_service));
}

void mf::ProtobufAsioCommunicator::stop()
{
}

mf::ProtobufAsioCommunicator::~ProtobufAsioCommunicator()
{
    io_service.stop();

    if (io_service_thread.joinable())
    {
        io_service_thread.join();
    }

    connected_sessions.clear();

    std::remove(socket_file.c_str());
}

void mf::ProtobufAsioCommunicator::on_new_connection(
    std::shared_ptr<detail::Session> const& session,
    const boost::system::error_code& ec)
{
    if (!ec)
    {
        connected_sessions.add(session);

        session->read_next_message();
    }
    start_accept();
}

mfd::Session::Session(
    boost::asio::io_service& io_service,
    int id_,
    ConnectedSessions* connected_sessions,
    std::shared_ptr<protobuf::DisplayServer> const& display_server)
    : socket(io_service),
    id_(id_),
    connected_sessions(connected_sessions),
    display_server(display_server)
{
}

void mfd::Session::read_next_message()
{
    boost::asio::async_read(socket,
        boost::asio::buffer(message_header_bytes),
        boost::bind(&mfd::Session::on_read_size,
            this, ba::placeholders::error));
}

void mfd::Session::on_read_size(const boost::system::error_code& ec)
{
    if (!ec)
    {
        size_t const body_size = (message_header_bytes[0] << 8) + message_header_bytes[1];
        // Read newline delimited messages for now
        ba::async_read(
             socket,
             message,
             boost::asio::transfer_exactly(body_size),
             boost::bind(&Session::on_new_message,
                         this, ba::placeholders::error));
    }
}

void mfd::Session::on_new_message(const boost::system::error_code& ec)
{
    if (!ec)
    {
        /* Parse the client's message and handle it in the server.

           NOTE: the display_server member functions are synchronous, and
           execute callbacks directly. This means it is valid to pass stack
           variables into NewCallback() and (in the case of disconnect) to
           delete this session instance once the function returns.
        */
        std::istream in(&message);
        mir::protobuf::wire::Invocation invoke;

        invoke.ParseFromIstream(&in);

        if ("connect" == invoke.method_name())
        {
            mir::protobuf::ConnectMessage connect_message;
            connect_message.ParseFromString(invoke.parameters());

            display_server->connect(
                0,
                &connect_message,
                &surface,
                google::protobuf::NewCallback(
                    this,
                    &Session::send_response,
                    invoke.id(),
                    static_cast<google::protobuf::Message*>(&surface)));
        }
        else if ("create_surface" == invoke.method_name())
        {
            mir::protobuf::SurfaceParameters surface_message;
            surface_message.ParseFromString(invoke.parameters());

            display_server->create_surface(
                0,
                &surface_message,
                &surface,
                google::protobuf::NewCallback(
                    this,
                    &Session::send_response,
                    invoke.id(),
                    static_cast<google::protobuf::Message*>(&surface)));
        }
        else if ("release_surface" == invoke.method_name())
        {
            mir::protobuf::Void ignored;
            mir::protobuf::SurfaceId message;
            message.ParseFromString(invoke.parameters());

            display_server->release_surface(
                0,
                &message,
                &ignored,
                google::protobuf::NewCallback(
                    this,
                    &Session::send_response,
                    invoke.id(),
                    static_cast<google::protobuf::Message*>(&ignored)));
        }
        else if ("disconnect" == invoke.method_name())
        {
            mir::protobuf::Void ignored;
            ignored.ParseFromString(invoke.parameters());

            display_server->disconnect(
                0,
                &ignored,
                &ignored,
                google::protobuf::NewCallback(
                    this,
                    &Session::send_response,
                    invoke.id(),
                    static_cast<google::protobuf::Message*>(&ignored)));

            // Careful about what you do after this - it deletes this
            connected_sessions->remove(id());
            return;
        }
    }

    read_next_message();
}

void mfd::Session::on_response_sent(bs::error_code const& error, std::size_t)
{
    if (error)
        std::cerr << "ERROR sending response: " << error.message() << std::endl;
}

void mfd::Session::send_response(
    ::google::protobuf::uint32 id,
    google::protobuf::Message* response)
{
    std::ostringstream buffer1;
    response->SerializeToOstream(&buffer1);

    mir::protobuf::wire::Result result;
    result.set_id(id);
    result.set_response(buffer1.str());

    std::ostringstream buffer2;
    result.SerializeToOstream(&buffer2);

    const std::string& body = buffer2.str();
    const size_t size = body.size();
    const unsigned char header_bytes[2] =
    {
        static_cast<unsigned char>((size >> 8) & 0xff),
        static_cast<unsigned char>((size >> 0) & 0xff)
    };

    whole_message.resize(sizeof header_bytes + size);
    std::copy(header_bytes, header_bytes + sizeof header_bytes, whole_message.begin());
    std::copy(body.begin(), body.end(), whole_message.begin() + sizeof header_bytes);

    ba::async_write(
        socket,
        ba::buffer(whole_message),
        boost::bind(&Session::on_response_sent, this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}


mfd::ConnectedSessions::~ConnectedSessions()
{
}

void mfd::ConnectedSessions::add(std::shared_ptr<Session> const& session)
{
    sessions_list[session->id()] = session;
}

void mfd::ConnectedSessions::remove(int id)
{
    sessions_list.erase(id);
}

bool mfd::ConnectedSessions::includes(int id) const
{
    return sessions_list.find(id) != sessions_list.end();
}

void mfd::ConnectedSessions::clear()
{
    sessions_list.clear();
}
