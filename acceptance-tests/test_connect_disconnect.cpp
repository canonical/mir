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
 */

#include "mir/display_server.h"
#include "mir/frontend/application.h"
#include "mir/frontend/communicator.h"
#include "mir/process/posix_process.h"

#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mpx = mir::process::posix;
namespace geom = mir::geometry;

namespace {

class StubBufferAllocator : public mc::GraphicBufferAllocator
{
public:
    virtual std::shared_ptr<mc::Buffer> alloc_buffer(
        geom::Width, geom::Height, mc::PixelFormat)
    { 
        return nullptr;
    }
};

class StubBufferAllocationStrategy : public mc::BufferAllocationStrategy
{
public:
    StubBufferAllocationStrategy()
        : mc::BufferAllocationStrategy(
            std::shared_ptr<mc::GraphicBufferAllocator>(new StubBufferAllocator()))
    {
    }

    virtual void allocate_buffers_for_bundle(
        geom::Width, geom::Height, mc::PixelFormat, mc::BufferBundle *)
    {
    }
};

struct StubCommunicator : public mf::Communicator
{
    StubCommunicator()
    {
    }

    void run()
    {
    }
};

}

int test_exit()
{
    return ::testing::Test::HasFailure() ? EXIT_FAILURE : EXIT_SUCCESS;
}

TEST(client_server_communication, client_connects_and_disconnects)
{
    auto server_bind_and_connect = []() -> void
    {
        SCOPED_TRACE("Server");
        StubBufferAllocationStrategy strategy;
        mir::DisplayServer display_server(&strategy);
        display_server.run();
    };

    std::shared_ptr<mpx::Process> server =
            mpx::fork_and_run_in_a_different_process(
                server_bind_and_connect, test_exit);

    std::shared_ptr<mf::Communicator> communicator(new StubCommunicator());

    mir::frontend::Application application(communicator);

    auto client_connects_and_disconnects = [&]() -> void
    {
        SCOPED_TRACE("Client");
        EXPECT_NO_THROW(application.connect());
        EXPECT_NO_THROW(application.disconnect());
    };

    std::shared_ptr<mpx::Process> client =
            mpx::fork_and_run_in_a_different_process(
                client_connects_and_disconnects, test_exit);

    EXPECT_TRUE(client->wait_for_termination().is_successful());

    server->terminate();
    EXPECT_TRUE(server->wait_for_termination().is_successful());
}
