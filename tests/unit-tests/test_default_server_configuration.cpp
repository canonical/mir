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

#include "mir/test/doubles/mock_console_services.h"

#include <mir/default_server_configuration.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mtd = mir::test::doubles;

namespace
{

struct ServerConfig : public mir::DefaultServerConfiguration
{
    using mir::DefaultServerConfiguration::DefaultServerConfiguration;
    std::shared_ptr<mir::ConsoleServices> the_console_services() override
    {
        return mock_console_services;
    }

    std::shared_ptr<mtd::MockConsoleServices> const mock_console_services =
        std::make_shared<mtd::MockConsoleServices>();
};

}

struct DefaultServerConfigurationTest : testing::Test
{
};

TEST_F(DefaultServerConfigurationTest, creates_vt_switcher_by_default)
{
    char const* argv[] = {"test"};
    auto server_configuration = ServerConfig(1, argv);

    EXPECT_CALL(*server_configuration.mock_console_services, create_vt_switcher()).Times(1);

    server_configuration.the_event_filter_chain_dispatcher();
}

TEST_F(DefaultServerConfigurationTest, does_not_create_vt_switcher_when_disabled)
{
    char const* argv[] = {"test", "--vt-switching=false"};
    auto server_configuration = ServerConfig(2, argv);

    EXPECT_CALL(*server_configuration.mock_console_services, create_vt_switcher()).Times(0);

    server_configuration.the_event_filter_chain_dispatcher();
}

TEST_F(DefaultServerConfigurationTest, proceeds_when_failed_to_create_vt_switcher)
{
    char const* argv[] = {"test"};
    auto server_configuration = ServerConfig(1, argv);

    EXPECT_CALL(*server_configuration.mock_console_services, create_vt_switcher())
        .WillOnce(testing::Throw(std::runtime_error("No VT switching support available")));

    server_configuration.the_event_filter_chain_dispatcher();
}
