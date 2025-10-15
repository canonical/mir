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

#ifndef MIR_TESTS_INCLUDE_MIR_TEST_FRAMEWORK_ASYNC_SERVER_RUNNER_H_
#define MIR_TESTS_INCLUDE_MIR_TEST_FRAMEWORK_ASYNC_SERVER_RUNNER_H_

#include <mir_test_framework/temporary_environment_value.h>
#include <mir/test/auto_unblock_thread.h>

#include <mir/server.h>
#include <miral/set_window_management_policy.h>

#include <condition_variable>
#include <list>
#include <mutex>
#include <thread>

namespace mir_test_framework
{
class AsyncServerRunner
{
public:
    AsyncServerRunner();
    ~AsyncServerRunner() noexcept;

    void add_to_environment(char const* key, char const* value);

    /// Starts the server on a new thread
    void start_server();

    /// Stops the server and joins thread
    void stop_server();

    /// Wait for the server to exit and joins thread
    void wait_for_server_exit();

    mir::Server server;

protected:
    std::string const output_filename;

private:
    std::list<TemporaryEnvironmentValue> env;
    mir::test::AutoJoinThread server_thread;

    std::mutex mutex;
    std::condition_variable started;
    bool server_running{false};
};
}

#endif /* MIR_TESTS_INCLUDE_MIR_TEST_FRAMEWORK_ASYNC_SERVER_RUNNER_H_ */
