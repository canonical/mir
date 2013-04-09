/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/compositor.h"

#include "mir_test_framework/testing_server_configuration.h"
#include "mir_test_doubles/mock_input_manager.h"
#include "mir/run_mir.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <sys/types.h>
#include <unistd.h>

namespace mi = mir::input;
namespace mc = mir::compositor;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{

class MockCompositor : public mc::Compositor
{
public:
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(stop, void());
};

class ServerConfig : public mtf::TestingServerConfiguration
{
public:
    std::shared_ptr<mi::InputManager> the_input_manager() override
    {
        if (!mock_input_manager)
        {
            mock_input_manager = std::make_shared<mtd::MockInputManager>();

            EXPECT_CALL(*mock_input_manager, start()).Times(1);
            EXPECT_CALL(*mock_input_manager, stop()).Times(1);
        }

        return mock_input_manager;
    }

    std::shared_ptr<mc::Compositor> the_compositor() override
    {
        if (!mock_compositor)
        {
            mock_compositor = std::make_shared<MockCompositor>();

            EXPECT_CALL(*mock_compositor, start()).Times(1);
            EXPECT_CALL(*mock_compositor, stop()).Times(1);
        }

        return mock_compositor;
    }

private:
    std::shared_ptr<mtd::MockInputManager> mock_input_manager;
    std::shared_ptr<MockCompositor> mock_compositor;
};

}

TEST(DisplayServerMainLoopEvents, display_server_shuts_down_properly_on_sigint)
{
    ServerConfig server_config;

    mir::run_mir(server_config,
                 [](mir::DisplayServer&)
                 {
                    kill(getpid(), SIGINT);
                 });
}

TEST(DisplayServerMainLoopEvents, display_server_shuts_down_properly_on_sigterm)
{
    ServerConfig server_config;

    mir::run_mir(server_config,
                 [](mir::DisplayServer&)
                 {
                    kill(getpid(), SIGTERM);
                 });
}
