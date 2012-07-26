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

#include <boost/asio.hpp>

namespace mf = mir::frontend;

mf::ProtobufAsioCommunicator::ProtobufAsioCommunicator(std::string const& socket_file)
        : socket_file(socket_file)
{
    namespace ba = boost::asio;
    namespace bal = boost::asio::local;

    ba::io_service io_service;
    std::remove(socket_file.c_str());
    bal::stream_protocol::acceptor acceptor(io_service, socket_file);
    bal::stream_protocol::socket socket(io_service);
    acceptor.accept(socket);
}

mf::ProtobufAsioCommunicator::~ProtobufAsioCommunicator()
{
    std::remove(socket_file.c_str());
}
