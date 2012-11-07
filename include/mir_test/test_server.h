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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Alan Griffiths <alan@octopull.co.uk>
 */
#ifndef MIR_TEST_TEST_SERVER_H_
#define MIR_TEST_TEST_SERVER_H_

#include "mir_test/stub_server_tool.h"
#include "mir_test/mock_ipc_factory.h"
#include "src/frontend/protobuf_socket_communicator.h"

namespace mir
{
namespace test
{

struct TestServer
{
    TestServer(std::string socket_name,
               const std::shared_ptr<protobuf::DisplayServer>& tool) :
        factory(std::make_shared<MockIpcFactory>(*tool)),
        comm(socket_name, factory)
    {
    }

    // "Server" side
    std::shared_ptr<MockIpcFactory> factory;
    frontend::ProtobufSocketCommunicator comm;
};

}
}
#endif /* MIR_TEST_TEST_SERVER_H_ */
