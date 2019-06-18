/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_TEST_SERVER_H
#define MIRAL_TEST_SERVER_H

#include <miral/minimal_window_manager.h>
#include <miral/runner.h>
#include <miral/window_manager_tools.h>

#include <mir/test/auto_unblock_thread.h>
#include <mir_test_framework/temporary_environment_value.h>

#include <gtest/gtest.h>

#include <condition_variable>
#include <list>
#include <mutex>

namespace mir { namespace shell { class WindowManager; }}
namespace mir { namespace client { class Connection; }}
namespace miral
{
class WindowManagementPolicy;
class TestRuntimeEnvironment
{
public:
    void add_to_environment(char const* key, char const* value);

private:
    std::list<mir_test_framework::TemporaryEnvironmentValue> env;
};

struct TestDisplayServer : private TestRuntimeEnvironment
{
    TestDisplayServer();
    virtual ~TestDisplayServer();

    void start_server();
    void stop_server();

    auto connect_client(std::string name) -> mir::client::Connection;

    using TestRuntimeEnvironment::add_to_environment;

    MirRunner runner;

    // Passed to runner.run_with() by start_server()
    std::function<void(mir::Server&)> init_server = [](auto&){};

    void invoke_tools(std::function<void(WindowManagerTools& tools)> const& f);
    void invoke_window_manager(std::function<void(mir::shell::WindowManager& wm)> const& f);

    struct TestWindowManagerPolicy;
    virtual auto build_window_manager_policy(WindowManagerTools const& tools) -> std::unique_ptr<TestWindowManagerPolicy>;

private:
    WindowManagerTools tools{nullptr};
    std::weak_ptr<mir::shell::WindowManager> window_manager;
    mir::test::AutoJoinThread server_thread;
    std::mutex mutex;
    std::condition_variable started;
    mir::Server* server_running{nullptr};
};

struct TestServer : TestDisplayServer, testing::Test
{
    void SetUp() override;
    void TearDown() override;
};

struct TestDisplayServer::TestWindowManagerPolicy : MinimalWindowManager
{
    TestWindowManagerPolicy(WindowManagerTools const& tools, TestDisplayServer& test_fixture);
};

}

#endif //MIRAL_TEST_SERVER_H
