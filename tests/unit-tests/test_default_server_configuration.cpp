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

#include "mir/test/doubles/mock_configuration.h"
#include "mir/test/doubles/mock_console_services.h"
#include "mir/test/doubles/mock_option.h"

#include <mir/default_server_configuration.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mtd = mir::test::doubles;

using namespace ::testing;

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
    DefaultServerConfigurationTest()
    {
        EXPECT_CALL(*mock_option, get(mir::options::scene_report_opt))
            .WillOnce(Invoke(
                    [this](char const*) -> boost::any const&
                    {
                        return any_off;
                    }));
        EXPECT_CALL(*mock_option, get(mir::options::vt_switching_option_name))
            .WillOnce(Invoke(
                    [this](char const*) -> boost::any const&
                    {
                        return vt_switching;
                    }));
        ON_CALL(*mock_configuration, global_options())
            .WillByDefault(Return(mock_option));
    }

    std::shared_ptr<NiceMock<mtd::MockOption>> mock_option{std::make_shared<NiceMock<mtd::MockOption>>()};
    std::shared_ptr<NiceMock<mtd::MockConfiguration>> mock_configuration{std::make_shared<NiceMock<mtd::MockConfiguration>>()};

    boost::any vt_switching = true;
    boost::any const any_off = std::string{"off"};
};

TEST_F(DefaultServerConfigurationTest, creates_vt_switcher_by_default)
{
    auto server_configuration = ServerConfig(mock_configuration);

    EXPECT_CALL(*server_configuration.mock_console_services, create_vt_switcher()).Times(1);

    server_configuration.the_event_filter_chain_dispatcher();
}

TEST_F(DefaultServerConfigurationTest, does_not_create_vt_switcher_when_disabled)
{
    auto server_configuration = ServerConfig(mock_configuration);
    vt_switching = false;

    EXPECT_CALL(*server_configuration.mock_console_services, create_vt_switcher()).Times(0);

    server_configuration.the_event_filter_chain_dispatcher();
}

TEST_F(DefaultServerConfigurationTest, proceeds_when_failed_to_create_vt_switcher)
{
    auto server_configuration = ServerConfig(mock_configuration);

    EXPECT_CALL(*server_configuration.mock_console_services, create_vt_switcher())
        .WillOnce(testing::Throw(std::runtime_error("No VT switching support available")));

    server_configuration.the_event_filter_chain_dispatcher();
}
