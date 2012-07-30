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

//TODO Session should probably be an interface so that it can be mocked.
class Session;

class ProtobufAsioCommunicator : public Communicator
{
public:
    typedef boost::signals2::signal<void(std::shared_ptr<Session> const&)> NewSessionSignal;

    // Create communicator based on Boost asio and Google protobufs
    // using the supplied socket.
    ProtobufAsioCommunicator(std::string const& socket_file);

    ~ProtobufAsioCommunicator();

    void start();

    NewSessionSignal& signal_new_session();

private:
    void start_accept();
    void on_new_connection(std::shared_ptr<Session> const& session, const boost::system::error_code& ec);

    std::string const socket_file;
    boost::asio::io_service io_service;
    boost::asio::local::stream_protocol::acceptor acceptor;
    std::thread io_service_thread;
    NewSessionSignal new_session_signal;

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
        : socket(io_service), id_(id_)
    {
    }

private:
    // I wish for "friend void ProtobufAsioCommunicator::start_accept();" but that's private.
    friend class ProtobufAsioCommunicator;
    boost::asio::local::stream_protocol::socket socket;
    int const id_;
};
}
}

#endif // MIR_FRONTEND_PROTOBUF_ASIO_COMMUNICATOR_H_
