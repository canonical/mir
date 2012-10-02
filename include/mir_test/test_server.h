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

#include "mir_test/stub_server.h"
#include "mir_test/mock_ipc_factory.h"
#include "mir/frontend/protobuf_asio_communicator.h"

namespace mir
{
namespace test
{

struct TestServer
{
    TestServer(std::string socket_name) :
        factory(std::make_shared<MockIpcFactory>(stub_services)),
        comm(socket_name, factory)
    {
    }

    void expect_surface_count(int expected_count)
    {
        std::unique_lock<std::mutex> ul(stub_services.guard);
        while (stub_services.surface_count != expected_count)
            stub_services.wait_condition.wait(ul);

        EXPECT_EQ(expected_count, stub_services.surface_count);
    }

    // "Server" side
    StubServer stub_services;
    std::shared_ptr<MockIpcFactory> factory;
    frontend::ProtobufAsioCommunicator comm;
};

}
}
#endif /* MIR_TEST_TEST_SERVER_H_ */
