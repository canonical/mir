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


#ifndef MIR_TEST_TESTING_SERVER_CONFIGURATION_H_
#define MIR_TEST_TESTING_SERVER_CONFIGURATION_H_

#include "mir/server_status_listener.h"
#include "mir_test_framework/stubbed_server_configuration.h"

#include "mir/test/cross_process_sync.h"

namespace mir_test_framework
{
using namespace mir;

class TemporaryEnvironmentValue;

struct TestingServerStatusListener : public mir::ServerStatusListener
{
    TestingServerStatusListener(mir::test::CrossProcessSync const& sync,
                                std::function<void(void)> const& on_start)
        : server_started_sync{sync},
          on_start{on_start}
    {
    }

    void paused() {}
    void resumed() {}
    void started()
    {
        server_started_sync.try_signal_ready_for();
        on_start();
    }
    void ready_for_user_input() {}
    void stop_receiving_input() {}

    mir::test::CrossProcessSync server_started_sync;
    std::function<void(void)> const on_start;
};

class TestingServerConfiguration : public StubbedServerConfiguration
{
public:
    TestingServerConfiguration();
    explicit TestingServerConfiguration(std::vector<geometry::Rectangle> const& display_rects);
    TestingServerConfiguration(
        std::vector<geometry::Rectangle> const& display_rects,
        std::vector<std::unique_ptr<TemporaryEnvironmentValue>>&& setup_environment);
    ~TestingServerConfiguration();

    // Code to run in server process
    virtual void exec();

    // Code to run in server process after server starts
    virtual void on_start();

    // Code to run in server process after server exits
    virtual void on_exit();

    std::shared_ptr<mir::ServerStatusListener> the_server_status_listener() override;

    using DefaultServerConfiguration::the_options;

    virtual void wait_for_server_start();

private:
    mir::test::CrossProcessSync server_started_sync;
    bool using_server_started_sync;
    std::vector<std::unique_ptr<TemporaryEnvironmentValue>> environment;
};
}

#endif /* MIR_TEST_TESTING_SERVER_CONFIGURATION_H_ */
