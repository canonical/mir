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

#ifndef MIR_TEST_FRAMEWORK_IN_PROCESS_SERVER_H_
#define MIR_TEST_FRAMEWORK_IN_PROCESS_SERVER_H_

#include <gtest/gtest.h>

#include <string>
#include <thread>

namespace mir
{
class DisplayServer;
class DefaultServerConfiguration;
}


namespace mir_test_framework
{
/// Fixture for running Mir server in test process
struct InProcessServer : testing::Test
{
    ~InProcessServer();

    /// Starts the server
    /// \warning don't forget to call this if you override SetUp()
    void SetUp() override;

    /// Stops the server
    /// \warning don't forget to call this if you override TearDown()
    void TearDown() override;

    /// \return a connection string for a new client to connect to the server
    std::string new_connection();

private:
    mir::DisplayServer* start_mir_server();
    virtual mir::DefaultServerConfiguration& server_config() = 0;

    std::thread server_thread;
    mir::DisplayServer* display_server = 0;
};
}

#endif /* MIR_TEST_FRAMEWORK_IN_PROCESS_SERVER_H_ */
