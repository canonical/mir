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
 * Authored by: Thomas Guest <thomas.guest@canonical.com>
 */

#ifndef MIR_TESTING_PROCESS_MANAGER
#define MIR_TESTING_PROCESS_MANAGER

#include "mir/server_configuration.h"

#include "mir/chrono/chrono.h"
#include "mir/process/process.h"

#include <memory>
#include <list>

namespace mir
{
class DisplayServer;

struct TestingClientConfiguration
{
    // Code to run in client process
    virtual void exec() = 0;
};

struct TestingServerConfiguration : DefaultServerConfiguration
{
    // Code to run in server process
    virtual void exec(DisplayServer* display_server);

    // Code to run in server process after server exits
    virtual void on_exit(DisplayServer* display_server);

    virtual std::string const& default_socket_file();
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
    void os_signal_handler(int signal);

private:

    std::shared_ptr<mir::process::Process> server_process;
    std::list<std::shared_ptr<mir::process::Process>> clients;

    bool is_test_process;
};

std::string const& test_socket_file();

bool detect_server(
        const std::string& socket_file,
        std::chrono::milliseconds const& timeout);

}

#endif // MIR_TESTING_PROCESS_MANAGER
