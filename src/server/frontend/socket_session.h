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


#ifndef MIR_FRONTEND_DETAIL_SOCKET_SESSION_H_
#define MIR_FRONTEND_DETAIL_SOCKET_SESSION_H_

#include "message_processor.h"
#include "connected_sessions.h"

#include <boost/asio.hpp>

#include <sys/types.h>

namespace mir
{
namespace frontend
{
namespace detail
{

class MessageReceiver;

struct SocketSession
{
    SocketSession(
        std::shared_ptr<MessageReceiver> const& socket_receiver,
        int id_,
        std::shared_ptr<ConnectedSessions<SocketSession>> const& connected_sessions,
        std::shared_ptr<MessageProcessor> const& processor);

    ~SocketSession() noexcept;

    int id() const { return id_; }

    void read_next_message();

private:
    void on_response_sent(boost::system::error_code const& error, std::size_t);
    void on_new_message(const boost::system::error_code& ec);
    void on_read_size(const boost::system::error_code& ec);

    std::shared_ptr<MessageReceiver> const socket_receiver;
    int const id_;
    std::shared_ptr<ConnectedSessions<SocketSession>> const connected_sessions;
    std::shared_ptr<MessageProcessor> processor;

    boost::asio::streambuf header;
    boost::asio::streambuf message;
};

}
}
}

#endif /* MIR_FRONTEND_DETAIL_SOCKET_SESSION_H_ */
