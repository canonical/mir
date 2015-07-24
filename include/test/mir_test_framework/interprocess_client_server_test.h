/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_FRAMEWORK_INTERPROCESS_CLIENT_SERVER_TEST_H_
#define MIR_TEST_FRAMEWORK_INTERPROCESS_CLIENT_SERVER_TEST_H_

#include "mir_test_framework/headless_test.h"
#include "mir/test/cross_process_sync.h"

namespace mir_test_framework
{
class Process;
class Result;

class InterprocessClientServerTest : public HeadlessTest
{
public:
    char const* const mir_test_socket = test_socket_file().c_str();

    ~InterprocessClientServerTest();

    void init_server(std::function<void()> const& init_code);

    void run_in_server(std::function<void()> const& exec_code);

    void run_in_client(std::function<void()> const& client_code);

    auto new_client_process(std::function<void()> const& client_code) -> std::shared_ptr<Process>;

    bool is_test_process() const;

    pid_t client_pid() const { return client_process_id; }

    void TearDown() override;

    // Convenient [test|server|client] identifier if adding debug messages
    auto process_type() const -> char const* { return process_tag; }

    void expect_server_signalled(int signal);

    void stop_server();

    bool sigkill_server_process();

    Result wait_for_shutdown_server_process();

private:

    pid_t test_process_id{getpid()};
    pid_t server_process_id{0};
    pid_t client_process_id{0};
    std::shared_ptr<Process> server_process;
    std::unique_ptr<mir::test::CrossProcessSync> shutdown_sync{new mir::test::CrossProcessSync()};
    char const* process_tag = "test";
    std::function<void()> server_setup = []{};
    bool server_signal_expected{false};
    int expected_server_failure_signal{0};
};
}

#endif /* MIR_TEST_FRAMEWORK_INTERPROCESS_CLIENT_SERVER_TEST_H_ */
