/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIRAL_TEST_DISPLAY_SERVER_H
#define MIRAL_TEST_DISPLAY_SERVER_H

#include <miral/window_management_policy.h>
#include <miral/runner.h>
#include <miral/window_manager_tools.h>

#include <mir/test/auto_unblock_thread.h>
#include <mir_test_framework/temporary_environment_value.h>

#include <condition_variable>
#include <list>
#include <mutex>

namespace mir { namespace shell { class WindowManager; }}
namespace miral
{
class WindowManagementPolicy;

class TestRuntimeEnvironment
{
public:
    TestRuntimeEnvironment();
    void add_to_environment(char const* key, char const* value);

private:
    std::list <mir_test_framework::TemporaryEnvironmentValue> env;
};

struct TestDisplayServer : private TestRuntimeEnvironment
{
    TestDisplayServer();
    TestDisplayServer(int argc, char const** argv);

    virtual ~TestDisplayServer();

    /// Add an environment variable for the duration of the test run
    using TestRuntimeEnvironment::add_to_environment;

    /// Add a callback to be invoked when the server has started,
    /// If multiple callbacks are added they will be invoked in the sequence added.
    /// \note call before start_server()
    void add_start_callback(std::function<void()> const& start_callback);

    /// Add a callback to be invoked when the server is about to stop,
    /// If multiple callbacks are added they will be invoked in the reverse sequence added.
    /// \note call before start_server()
    void add_stop_callback(std::function<void()> const& stop_callback);

    /// Add signal handler to the server's main loop.
    void register_signal_handler(std::initializer_list<int> signals, std::function<void(int)> const& handler);

    /// Add a watch on a file descriptor. 
    /// The handler will be triggered when there is data to read on the Fd.
    auto register_fd_handler(mir::Fd fd, std::function<void(int)> const& handler)
    -> std::unique_ptr<miral::FdHandle>;

    /// Set a handler for exceptions caught in run_with().
    /// The default action is to call mir::report_exception(std::cerr)
    /// \note call before start_server()
    void set_exception_handler(std::function<void()> const& handler);

    /// Add configuration code to be passed to runner.run_with() by start_server()
    /// \note call before start_server()
    void add_server_init(std::function<void(mir::Server&)>&& init);

    /// Start the server
    /// \note Typically called by TestServer::SetUp()
    void start_server();

    /// Wrapper to gain access to WindowManagerTools API (with correct locking in place)
    /// \note call after start_server()
    void invoke_tools(std::function<void(WindowManagerTools& tools)> const& f);

    /// Stop the server
    /// \note Typically called by TestServer::TearDown()
    void stop_server();

    virtual auto
    build_window_manager_policy(WindowManagerTools const& tools) -> std::unique_ptr<WindowManagementPolicy>;

    /// Wrapper to gain access to the MirRunner
    /// \note call after start_server()
    void invoke_runner(std::function<void(MirRunner& runner)> const& f);

    mir::Server& server() const { return *server_running; }
private:
    MirRunner runner;
    mir::Server* server_running{nullptr};

    WindowManagerTools tools{nullptr};
    mir::test::AutoJoinThread server_thread;
    std::mutex mutex;
    std::condition_variable started;
    std::function<void(mir::Server&)> init_server = [](auto&){};
};
}
#endif //MIRAL_TEST_DISPLAY_SERVER_H
