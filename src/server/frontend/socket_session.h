/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_FRONTEND_DETAIL_SOCKET_SESSION_H_
#define MIR_FRONTEND_DETAIL_SOCKET_SESSION_H_

#include "message_processor.h"
#include "connected_sessions.h"

#include <boost/asio.hpp>

namespace mir
{
namespace frontend
{
namespace detail
{

struct SocketSession : public MessageSender
{
    SocketSession(
        boost::asio::io_service& io_service,
        int id_,
        std::shared_ptr<ConnectedSessions<SocketSession>> const& connected_sessions) :
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

    boost::asio::local::stream_protocol::socket& get_socket()
    {
        return socket;
    }

private:
    void send(std::string const& body);
    void send_fds(std::vector<int32_t> const& fd);

    void on_response_sent(boost::system::error_code const& error, std::size_t);
    void on_new_message(const boost::system::error_code& ec);
    void on_read_size(const boost::system::error_code& ec);

    boost::asio::local::stream_protocol::socket socket;
    int const id_;
    std::shared_ptr<ConnectedSessions<SocketSession>> const connected_sessions;
    std::shared_ptr<MessageProcessor> processor;
    boost::asio::streambuf message;
    static size_t const size_of_header = 2;
    unsigned char message_header_bytes[size_of_header];
    std::vector<char> whole_message;
};

}
}
}

#endif /* MIR_FRONTEND_DETAIL_SOCKET_SESSION_H_ */
