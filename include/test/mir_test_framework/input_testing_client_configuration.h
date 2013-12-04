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
 * Authored by: Robert Carr <robert.carr@canonical.com>n
 */

#ifndef MIR_TEST_FRAMEWORK_INPUT_TESTING_CLIENT_CONFIGURATION
#define MIR_TEST_FRAMEWORK_INPUT_TESTING_CLIENT_CONFIGURATION

#include "mir_test_framework/testing_client_configuration.h"
#include "mir_test_framework/cross_process_sync.h"
#include "mir_test/wait_condition.h"

#include <mir_toolkit/event.h>

#include <gmock/gmock.h>

#include <string>

namespace mir_test_framework
{

/// A fixture to be used with InputTestingServerConfiguration for input acceptance testing scenarios.
/// By default, the client will connect and a surface will be created.
/// The framework ensures the server will not send events before client is ready through CrossProcessSync.
class InputTestingClientConfiguration : public TestingClientConfiguration
{
public:
    InputTestingClientConfiguration(std::string const& client_name, CrossProcessSync const& input_cb_setup_fence);
    virtual ~InputTestingClientConfiguration() = default;

    void exec();

    struct MockInputHandler
    {
        MOCK_METHOD1(handle_input, void(MirEvent const*));
    };
    // This function will be called at an appropriate time for input expectations to be set.
    // on handler. It is expected that mt::WakeUp(all_events_received) will be triggered by
    // the last expectation, as this is what triggers the verification of the Mock and 
    // termination of the testing client.
    virtual void expect_input(MockInputHandler &handler, mir::test::WaitCondition& all_events_received) = 0;

    // This fixture is intended to be used with InputTestingServer
    // which allows for setting surface sizes as part of the
    // input-testing shell.
    static int const surface_width = 100;
    static int const surface_height = 100;
private:
    std::string const client_name;
    CrossProcessSync input_cb_setup_fence;
};

}
#endif /* MIR_TEST_FRAMEWORK_INPUT_TESTING_CLIENT_CONFIGURATION */
