/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#define MIR_LOG_COMPONENT "test_mir_runner_access"

#include "mir_test_framework/window_management_test_harness.h"
#include <miral/minimal_window_manager.h>
#include <mir/geometry/rectangle.h>
#include <mir/graphics/display_configuration.h>
#include <mir/fd.h>

#include <gtest/gtest.h>
#include <atomic>
#include <signal.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
class MirRunnerAccessTest : public mir_test_framework::WindowManagementTestHarness
{
public:
    auto get_builder() -> mir_test_framework::WindowManagementPolicyBuilder override
    {
        return [&](miral::WindowManagerTools const& tools)
        {
            return std::make_unique<miral::MinimalWindowManager>(tools);
        };
    }

    auto get_initial_output_configs() -> std::vector<mg::DisplayConfigurationOutput> override
    {
        return output_configs_from_output_rectangles({
            geom::Rectangle{{0, 0}, {1920, 1080}}
        });
    }
};

// Test that start callbacks can be registered
TEST_F(MirRunnerAccessTest, start_callback_can_be_registered)
{
    // Test that the API is available and doesn't crash
    add_start_callback([]
    {
        // This callback won't be invoked since WindowManagementTestHarness
        // doesn't use MirRunner::run_with(), but the API should be available
    });
    
    // The test passes if no exception is thrown
}

// Test that multiple start callbacks can be registered
TEST_F(MirRunnerAccessTest, multiple_start_callbacks_can_be_registered)
{
    // Test that multiple callbacks can be registered without crashing
    add_start_callback([] { });
    add_start_callback([] { });
    add_start_callback([] { });
    
    // The test passes if no exception is thrown
}

// Test that stop callbacks can be registered
TEST_F(MirRunnerAccessTest, stop_callback_can_be_registered)
{
    // Test that the API is available and doesn't crash
    add_stop_callback([]
    {
        // This callback won't be invoked since WindowManagementTestHarness
        // doesn't use MirRunner::run_with(), but the API should be available
    });
    
    // The test passes if no exception is thrown
}

// Test that signal handlers can be registered
TEST_F(MirRunnerAccessTest, signal_handler_can_be_registered)
{
    // Test that the API is available and doesn't crash
    register_signal_handler({SIGUSR1}, [](int)
    {
        // This handler won't be invoked since WindowManagementTestHarness
        // doesn't use MirRunner::run_with(), but the API should be available
    });
    
    // The test passes if no exception is thrown
}

// Test that fd handlers can be registered
TEST_F(MirRunnerAccessTest, fd_handler_can_be_registered)
{
    int pipefd[2];
    ASSERT_EQ(0, pipe(pipefd));
    
    // Test that the API is available and returns a valid handle
    auto handle = register_fd_handler(mir::Fd{pipefd[0]}, [](int)
    {
        // This handler may or may not be invoked depending on MirRunner's
        // FdManager implementation, but the API should be available
    });
    
    EXPECT_NE(nullptr, handle);
    
    // Clean up
    close(pipefd[0]);
    close(pipefd[1]);
}

// Test that invoke_runner doesn't crash
TEST_F(MirRunnerAccessTest, invoke_runner_doesnt_crash)
{
    bool invoked = false;
    
    invoke_runner([&](miral::MirRunner&)
    {
        invoked = true;
    });
    
    EXPECT_TRUE(invoked);
}
}
