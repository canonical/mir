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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test/test_protobuf_server.h"
#include "src/server/frontend/protobuf_socket_communicator.h"

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mf = mir::frontend;


mt::TestProtobufServer::TestProtobufServer(
    std::string socket_name,
    const std::shared_ptr<protobuf::DisplayServer>& tool) :
    factory(std::make_shared<mtd::StubIpcFactory>(*tool)),
    comm(make_communicator(socket_name, factory))
{
}

std::shared_ptr<mf::Communicator> mt::TestProtobufServer::make_communicator(const std::string& socket_name, std::shared_ptr<frontend::ProtobufIpcFactory> const& factory)
{
    return std::make_shared<mf::ProtobufSocketCommunicator>(socket_name, factory, 10, []{});
}
