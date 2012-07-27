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

namespace ba = boost::asio;
namespace bal = boost::asio::local;


class ProtobufAsioCommunicator : public Communicator
{
public:
    typedef boost::signals2::signal<void()> NewSessionSignal;

    // Create communicator based on Boost asio and Google protobufs
    // using the supplied socket.
    ProtobufAsioCommunicator(std::string const& socket_file);

    ~ProtobufAsioCommunicator();

    void start();

    NewSessionSignal& signal_new_session();
private:
    void on_new_connection(const boost::system::error_code& ec);

    std::string const socket_file;
    ba::io_service io_service;
    bal::stream_protocol::acceptor acceptor;
    bal::stream_protocol::socket socket;
    std::thread io_service_thread;
    NewSessionSignal new_session_signal;
};

}
}

#endif // MIR_FRONTEND_PROTOBUF_ASIO_COMMUNICATOR_H_
