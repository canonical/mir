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

#include "server_connection_implementations.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;

mf::FileSocketConnection::FileSocketConnection(std::string const& filename) :
        filename(filename)
{
    std::remove(filename.c_str());
}

mf::FileSocketConnection::~FileSocketConnection()
{
    std::remove(filename.c_str());
}

boost::asio::local::stream_protocol::acceptor mf::FileSocketConnection::acceptor(boost::asio::io_service& io_service) const
{
    return {io_service, filename};
}

std::string mf::FileSocketConnection::client_uri() const
{
    return filename;
}

#include <iostream>
mf::SocketPairConnection::SocketPairConnection()
{
    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, socket_fd))
    {
        BOOST_THROW_EXCEPTION(
            boost::enable_error_info(
                std::runtime_error("Could not create socket pair")) << boost::errinfo_errno(errno));
    }

    char buffer[42];
    sprintf(buffer, "fd://%d", socket_fd[client]);
    filename = buffer;

    std::cerr << "DEBUG (" << __PRETTY_FUNCTION__ << "), server_fd=" << socket_fd[server] << ", client_fd=" << socket_fd[client] << std::endl;
}

mf::SocketPairConnection::~SocketPairConnection() noexcept
{
    std::cerr << "DEBUG (" << __PRETTY_FUNCTION__ << "), server_fd=" << socket_fd[server] << ", client_fd=" << socket_fd[client] << std::endl;
    close(socket_fd[client]);
    close(socket_fd[server]);
}

boost::asio::local::stream_protocol::acceptor mf::SocketPairConnection::acceptor(boost::asio::io_service& io_service) const
{
    return {io_service, boost::asio::local::stream_protocol(), socket_fd[server]};
}

std::string mf::SocketPairConnection::client_uri() const
{

    return filename;
}
