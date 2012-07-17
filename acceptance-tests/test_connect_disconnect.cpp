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
 *              Thomas Guest <thomas.guest@canonical.com>
 */

#include "display_server_test_environment.h"

#include "mir/frontend/application.h"
#include "mir/frontend/communicator.h"
#include "mir/process/process.h"

#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mp = mir::process;

namespace 
{    
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

TEST(DisplayServerTest, client_connects_and_disconnects)
{
    std::shared_ptr<mf::Communicator> communicator(new StubCommunicator());
    mf::Application application(communicator);

    auto client_connects_and_disconnects = [&]() -> void
    {
        SCOPED_TRACE("Client");
        EXPECT_NO_THROW(application.connect());
        EXPECT_NO_THROW(application.disconnect());
    };

    std::shared_ptr<mp::Process> client =
            mp::fork_and_run_in_a_different_process(
                client_connects_and_disconnects, test_exit);

    EXPECT_TRUE(client->wait_for_termination().succeeded());
}
