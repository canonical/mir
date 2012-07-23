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

#include "display_server_test_fixture.h"

#include "mir/frontend/application.h"
#include "mir/frontend/communicator.h"

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

void empty_function()
{
}

void client_connects_and_disconnects()
{
    std::shared_ptr<mf::Communicator> communicator(new StubCommunicator());
    mf::Application application(communicator);

    EXPECT_NO_THROW(application.connect());
    EXPECT_NO_THROW(application.disconnect());
}
}

TEST_F(DisplayServerTestFixture, client_connects_and_disconnects)
{
    launch_server_process(empty_function);
    launch_client_process(client_connects_and_disconnects);
}
