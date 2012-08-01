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

#ifndef MIR_FRONTEND_PROTOBUF_ASIO_COMMUNICATOR_H_
#define MIR_FRONTEND_PROTOBUF_ASIO_COMMUNICATOR_H_

#include "communicator.h"
#include "mir/thread/all.h"

#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include <string>

namespace mir
{
namespace frontend
{

class Session;

enum class SessionState
{
    initialised,
    connected,
    disconnected,
    error
};

class ProtobufAsioCommunicator : public Communicator
{
public:
    typedef boost::signals2::signal<void(std::shared_ptr<Session> const&, SessionState)> SessionStateSignal;

    // Create communicator based on Boost asio and Google protobufs
    // using the supplied socket.
    explicit ProtobufAsioCommunicator(std::string const& socket_file);

    ~ProtobufAsioCommunicator();

    void start();

    SessionStateSignal& signal_session_state();

private:
    void start_accept();
    void on_new_connection(std::shared_ptr<Session> const& session, const boost::system::error_code& ec);
    void on_new_message(std::shared_ptr<Session> const& session, const boost::system::error_code& ec);
    void read_next_message(std::shared_ptr<Session> const& session);
    void change_state(std::shared_ptr<Session> const& session, SessionState new_state);

    std::string const socket_file;
    boost::asio::io_service io_service;
    boost::asio::local::stream_protocol::acceptor acceptor;
    std::thread io_service_thread;
    SessionStateSignal session_state_signal;

    int next_id;
};

class Session
{
public:
    int id() const
    {
        return id_;
    }

    explicit Session(boost::asio::io_service& io_service, int id_)
        : socket(io_service)
        , id_(id_)
        , state(SessionState::initialised)
    {
    }

private:
    friend class ProtobufAsioCommunicator;

    boost::asio::local::stream_protocol::socket socket;
    boost::asio::streambuf message;
    int const id_;
    SessionState state;
};

}
}

#endif // MIR_FRONTEND_PROTOBUF_ASIO_COMMUNICATOR_H_
