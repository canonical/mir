/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#ifndef MIR_TESTING_PROCESS_MANAGER
#define MIR_TESTING_PROCESS_MANAGER

#include "mir/server_configuration.h"

#include "mir_test_framework/process.h"

#include "mir_test_framework/testing_server_configuration.h"

#include <memory>
#include <list>
#include <functional>

namespace mir
{
class DisplayServer;
}

namespace mir_test_framework
{
using namespace mir;

struct TestingClientConfiguration
{
    virtual ~TestingClientConfiguration() = default;
    // Code to run in client process
    virtual void exec() = 0;
};


class TestingProcessManager
{
public:
    TestingProcessManager();
    ~TestingProcessManager();

    void launch_server_process(TestingServerConfiguration& config);
    void launch_client_process(TestingClientConfiguration& config);

    void tear_down_clients();
    void tear_down_server();
    void tear_down_all();
    Result shutdown_server_process();
    void kill_client_processes();
    void terminate_client_processes();
    void run_in_test_process(std::function<void()> const& run_code);

private:
    std::shared_ptr<Process> server_process;
    std::list<std::shared_ptr<Process>> clients;

    bool is_test_process;
    bool server_process_was_started;
};

std::string const& test_socket_file();

}

#endif // MIR_TESTING_PROCESS_MANAGER
