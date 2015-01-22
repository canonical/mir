/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 */

#include "mir/scene/surface_configurator.h"
#include "mir/scene/surface.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface_configurator.h"
#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/any_surface.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

namespace
{

struct SurfaceCreatingClient : public mtf::TestingClientConfiguration
{
    void exec() override
    {
        connection = mir_connect_sync(mir_test_socket, __PRETTY_FUNCTION__);
        ASSERT_TRUE(connection != NULL);
        EXPECT_TRUE(mir_connection_is_valid(connection));
        EXPECT_STREQ(mir_connection_get_error_message(connection), "");

        surface = mtf::make_any_surface(connection);
    }

    MirConnection* connection;
    MirSurface* surface;
};

}

TEST_F(BespokeDisplayServerTestFixture, the_shell_surface_configurator_is_notified_of_attribute_changes)
{
    struct ServerConfiguration : TestingServerConfiguration
    {
        std::shared_ptr<ms::SurfaceConfigurator> the_surface_configurator() override
        {
            return mt::fake_shared(mock_configurator);
        }

        void exec() override
        {
            using namespace ::testing;

            EXPECT_CALL(mock_configurator, select_attribute_value(_, Ne(mir_surface_attrib_state), _))
                .Times(AnyNumber())
                .WillRepeatedly(ReturnArg<2>());
            EXPECT_CALL(mock_configurator, attribute_set(_, Ne(mir_surface_attrib_state), _)).Times(AnyNumber());

            ON_CALL(mock_configurator, select_attribute_value(_, _, _)).WillByDefault(Return(mir_surface_state_maximized));
            EXPECT_CALL(mock_configurator, select_attribute_value(_, mir_surface_attrib_state, Eq(mir_surface_state_maximized))).Times(1);
            EXPECT_CALL(mock_configurator, attribute_set(_, mir_surface_attrib_state, Eq(mir_surface_state_maximized))).Times(1);
        }

        mtd::MockSurfaceConfigurator mock_configurator;
    } server_config;
    launch_server_process(server_config);

    struct ClientConfiguration : SurfaceCreatingClient
    {
        void exec() override
        {
            SurfaceCreatingClient::exec();
            mir_wait_for(mir_surface_set_state(surface, mir_surface_state_maximized));
            EXPECT_EQ(mir_surface_state_maximized, mir_surface_get_state(surface));
        }
    } client_config;
    launch_client_process(client_config);
}

TEST_F(BespokeDisplayServerTestFixture, the_shell_surface_configurator_may_interfere_with_attribute_changes)
{
    struct ServerConfiguration : TestingServerConfiguration
    {
        std::shared_ptr<ms::SurfaceConfigurator> the_surface_configurator() override
        {
            return mt::fake_shared(mock_configurator);
        }
        void exec() override
        {
            using namespace ::testing;

            EXPECT_CALL(mock_configurator, select_attribute_value(_, Ne(mir_surface_attrib_state), _))
                .Times(AnyNumber())
                .WillRepeatedly(ReturnArg<2>());
            EXPECT_CALL(mock_configurator, attribute_set(_, Ne(mir_surface_attrib_state), _)).Times(AnyNumber());

            EXPECT_CALL(mock_configurator, select_attribute_value(_, mir_surface_attrib_state, Eq(mir_surface_state_maximized))).Times(1)
                .WillOnce(Return(mir_surface_state_maximized));
            EXPECT_CALL(mock_configurator, attribute_set(_, mir_surface_attrib_state, Eq(mir_surface_state_maximized))).Times(1);
        }
        mtd::MockSurfaceConfigurator mock_configurator;
    } server_config;
    launch_server_process(server_config);

    struct ClientConfiguration : SurfaceCreatingClient
    {
        void exec() override
        {
            SurfaceCreatingClient::exec();
            mir_wait_for(mir_surface_set_state(surface, mir_surface_state_maximized));
            EXPECT_EQ(mir_surface_state_maximized, mir_surface_get_state(surface));
        }
    } client_config;
    launch_client_process(client_config);
}
