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

#ifndef MIR_FRONTEND_SERVER_CONNECTION_IMPLEMENTATIONS_H_
#define MIR_FRONTEND_SERVER_CONNECTION_IMPLEMENTATIONS_H_

#include "mir/frontend/socket_connection.h"

namespace mir
{
namespace frontend
{
class FileSocketConnection : public SocketConnection
{
public:
    FileSocketConnection(std::string const& filename);

    ~FileSocketConnection();

    boost::asio::local::stream_protocol::acceptor acceptor(boost::asio::io_service& io_service) const;

    virtual std::string client_uri() const;

private:
    std::string const filename;
};

class SocketPairConnection : public SocketConnection
{
public:
    SocketPairConnection();

    ~SocketPairConnection() noexcept;

    boost::asio::local::stream_protocol::acceptor acceptor(boost::asio::io_service& io_service) const;

    virtual std::string client_uri() const;

private:
    enum { server, client, size };
    int socket_fd[size];
    std::string filename;
};
}
}

#endif /* MIR_FRONTEND_SERVER_CONNECTION_IMPLEMENTATIONS_H_ */
