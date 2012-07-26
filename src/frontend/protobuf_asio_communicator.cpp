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

#include <functional>

namespace mf = mir::frontend;

mf::ProtobufAsioCommunicator::ProtobufAsioCommunicator(std::string const& socket_file)
        : socket_file((std::remove(socket_file.c_str()), socket_file)),
          acceptor(io_service, socket_file),
          socket(io_service)
{

    acceptor.async_accept(
            socket,
            std::bind(
                    &ProtobufAsioCommunicator::on_new_connection,
                    this,
                    std::placeholders::_1));
}

mf::ProtobufAsioCommunicator::~ProtobufAsioCommunicator()
{
    std::remove(socket_file.c_str());
}

void mf::ProtobufAsioCommunicator::on_new_connection(const boost::system::error_code& ec)
{
    if (ec)
    {
        // TODO: React to error here.
        return;
    }
}
