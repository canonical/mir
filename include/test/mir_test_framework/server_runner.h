/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_FRAMEWORK_SERVER_RUNNER_H_
#define MIR_TEST_FRAMEWORK_SERVER_RUNNER_H_

#include <gtest/gtest.h>

#include <thread>
#include <memory>
#include <mutex>

namespace mir
{
class MainLoop;
class DefaultServerConfiguration;
}

namespace mir_test_framework
{
/// Utility for running Mir server in test process
struct ServerRunner
{
    ServerRunner();
    virtual ~ServerRunner();

    /// Starts the server
    /// The method is synchronous, i.e., it returns only after the server has started
    void start_server();

    /// Stops the server
    /// The method is synchronous, i.e., it returns only after the server has stopped
    void stop_server();

private:
    std::shared_ptr<mir::MainLoop> start_mir_server();
    virtual mir::DefaultServerConfiguration& server_config() = 0;

    std::thread server_thread;
    std::mutex main_loop_mutex;
    std::shared_ptr<mir::MainLoop> main_loop;
};
}

#endif /* MIR_TEST_FRAMEWORK_SERVER_RUNNER_H_ */
