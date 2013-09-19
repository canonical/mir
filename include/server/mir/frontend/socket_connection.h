/*
 * Copyright © 2013 Canonical Ltd.
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


#ifndef MIR_FRONTEND_SOCKET_H_
#define MIR_FRONTEND_SOCKET_H_

#include <boost/asio.hpp>
#include <string>

namespace mir
{
namespace frontend
{
class SocketConnection
{
public:
    virtual boost::asio::local::stream_protocol::acceptor acceptor(boost::asio::io_service& io_service) const = 0;
    virtual std::string client_uri() const = 0;

protected:
    SocketConnection() = default;
    virtual ~SocketConnection() = default;
    SocketConnection(SocketConnection const&) = delete;
    SocketConnection& operator=(SocketConnection const&) = delete;
};
}
}


#endif /* MIR_FRONTEND_SOCKET_H_ */
