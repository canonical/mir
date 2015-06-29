/*
 * Copyright © 2012-2015 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#include "mir_test_framework/deferred_in_process_server.h"
#include "mir_test_framework/testing_server_configuration.h"
#include "mir/test/doubles/mock_input_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{

struct ServerConfig : mtf::TestingServerConfiguration
{
    std::shared_ptr<mi::InputManager> the_input_manager() override
    {
        return mock_input_manager;
    }

    std::shared_ptr<mtd::MockInputManager> const mock_input_manager =
        std::make_shared<mtd::MockInputManager>();
};

struct InputManager : mtf::DeferredInProcessServer
{
    mtd::MockInputManager& the_mock_input_manager()
    {
        return *server_configuration.mock_input_manager;
    }

    mir::DefaultServerConfiguration& server_config() override
    {
        return server_configuration;
    }

    ServerConfig server_configuration;
};

}

TEST_F(InputManager, is_started_when_display_server_starts)
{
    EXPECT_CALL(the_mock_input_manager(), start()).Times(1);
    EXPECT_CALL(the_mock_input_manager(), stop()).Times(1);

    start_server();
}
