/*
 * Copyright © 2021 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include <miral/test_server.h>
#include <miral/display_configuration_option.h>
#include <miral/test_server.h>
#include <mir/graphics/display.h>
#include <mir/graphics/display_configuration.h>
#include <mir/test/fake_shared.h>
#include <mir/server.h>
#include <mir/graphics/display_configuration_observer.h>
#include <mir/observer_registrar.h>
#include <mir/shell/display_configuration_controller.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;
namespace mg = mir::graphics;
namespace ms = mir::scene;
namespace mt = mir::test;

namespace
{
struct DisplayConfigurationOption : miral::TestServer
{
    DisplayConfigurationOption()
    {
        start_server_in_setup = false;
    }
};

struct ConfigObserver : mg::DisplayConfigurationObserver
{
    std::shared_ptr<mg::DisplayConfiguration const> latest_config;

    void initial_configuration(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        latest_config = config;
    }

    void configuration_applied(std::shared_ptr<mg::DisplayConfiguration const> const& config) override
    {
        latest_config = config;
    }

    void base_configuration_updated(std::shared_ptr<mg::DisplayConfiguration const> const&) override {}
    void session_configuration_applied(std::shared_ptr<ms::Session> const&,
        std::shared_ptr<mg::DisplayConfiguration> const&) override {}
    void session_configuration_removed(std::shared_ptr<ms::Session> const&) override {}
    void configuration_failed(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override {}
    void catastrophic_configuration_error(
        std::shared_ptr<mg::DisplayConfiguration const> const&,
        std::exception const&) override {}
    void configuration_updated_for_session(
        std::shared_ptr<ms::Session> const&,
        std::shared_ptr<mg::DisplayConfiguration const> const&) override {}
};
}

TEST_F(DisplayConfigurationOption, client_connects)
{
    add_to_environment("MIR_SERVER_DISPLAY_SCALE", "2");
    mir::Server* the_server{nullptr};
    ConfigObserver config_observer;
    add_server_init([&](mir::Server& server)
        {
            the_server = &server;
        });
    add_server_init(miral::display_configuration_options);
    start_server();
    bool has_output{false};
    the_server->the_display_configuration_controller()->base_configuration()->for_each_output(
        [&](mg::UserDisplayConfigurationOutput const& output)
        {
            has_output = true;
            EXPECT_THAT(output.scale, Eq(2.0));
        });
    EXPECT_THAT(has_output, Eq(true));
}
