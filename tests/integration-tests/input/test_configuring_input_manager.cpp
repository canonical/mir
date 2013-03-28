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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/mock_input_manager.h"

namespace mi = mir::input;
namespace mtd = mir::test::doubles;

#include <stdio.h>

TEST_F(BespokeDisplayServerTestFixture, starting_display_server_starts_input_manager)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mi::InputManager> the_input_manager() override
        {

            if (!mock_input_manager.get())
            {
                mock_input_manager = std::make_shared<mtd::MockInputManager>();

                EXPECT_CALL(*mock_input_manager, start()).Times(1);
                EXPECT_CALL(*mock_input_manager, stop()).Times(1);
            }

            return mock_input_manager;
        }

        std::shared_ptr<mtd::MockInputManager> mock_input_manager;
    } server_config;

    launch_server_process(server_config);
}
