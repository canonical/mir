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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir/shell/session_container.h"
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/consuming_placement_strategy.h"
#include "mir/shell/organising_surface_factory.h"
#include "mir/shell/session_manager.h"
#include "mir/graphics/display.h"
#include "mir/shell/input_focus_selector.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_doubles/mock_focus_setter.h"
#include "mir_test_doubles/mock_input_focus_selector.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

namespace
{
struct ClientConfigCommon : TestingClientConfiguration
{
    ClientConfigCommon() :
        connection(0),
        surface(0)
    {
    }

    static void connection_callback(MirConnection* connection, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->connection = connection;
    }

    static void create_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_created(surface);
    }

    static void release_surface_callback(MirSurface* surface, void* context)
    {
        ClientConfigCommon* config = reinterpret_cast<ClientConfigCommon *>(context);
        config->surface_released(surface);
    }

    virtual void connected(MirConnection* new_connection)
    {
        connection = new_connection;
    }

    virtual void surface_created(MirSurface* new_surface)
    {
        surface = new_surface;
    }

    virtual void surface_released(MirSurface* /*released_surface*/)
    {
        surface = NULL;
    }

    MirConnection* connection;
    MirSurface* surface;
};

struct SurfaceCreatingClient : ClientConfigCommon
{
    void exec()
    {
        mir_wait_for(mir_connect(
            mir_test_socket,
            __PRETTY_FUNCTION__,
            connection_callback,
            this));
         ASSERT_TRUE(connection != NULL);
         MirSurfaceParameters const request_params =
        {
            __PRETTY_FUNCTION__,
            640, 480,
            mir_pixel_format_abgr_8888,
            mir_buffer_usage_hardware
        };
         mir_wait_for(mir_surface_create(connection, &request_params, NULL, create_surface_callback, this));
         mir_connection_release(connection);
    }
};

}

namespace
{
MATCHER(NonNullSession, "")
{
    return arg != std::shared_ptr<msh::Session>();
}
MATCHER(NonNullSessionTarget, "")
{
    return arg != std::shared_ptr<mi::SessionTarget>();
}
MATCHER(NonNullSurfaceTarget, "")
{
    return arg != std::shared_ptr<mi::SurfaceTarget>();
}
MATCHER(NullSurfaceTarget, "")
{
    return arg == std::shared_ptr<mi::SurfaceTarget>();
}
}

TEST_F(BespokeDisplayServerTestFixture, sessions_creating_surface_receive_focus)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mf::Shell>
        the_frontend_shell()
        {
            return session_manager(
            [this]() -> std::shared_ptr<msh::SessionManager>
            {
                using namespace ::testing;

                auto session_container = std::make_shared<msh::SessionContainer>();
                auto focus_setter = std::make_shared<mtd::MockFocusSetter>();
                auto focus_selection_strategy = std::make_shared<msh::RegistrationOrderFocusSequence>(session_container);

                auto placement_strategy = std::make_shared<msh::ConsumingPlacementStrategy>(the_display());
                auto organising_factory = std::make_shared<msh::OrganisingSurfaceFactory>(the_surface_factory(), placement_strategy);

                {
                    InSequence seq;
                    // Once on application registration and once on surface creation
                    EXPECT_CALL(*focus_setter, set_focus_to(NonNullSession())).Times(2);
                    // Focus is cleared when the session is closed
                    EXPECT_CALL(*focus_setter, set_focus_to(_)).Times(1);
                }
                // TODO: Counterexample ~racarr

                return std::make_shared<msh::SessionManager>(organising_factory, session_container, focus_selection_strategy, focus_setter);
            });
        }
    } server_config;

    launch_server_process(server_config);

    SurfaceCreatingClient client;

    launch_client_process(client);
}

TEST_F(BespokeDisplayServerTestFixture, surfaces_receive_input_focus_when_created)
{
    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<mtd::MockInputFocusSelector> focus_selector;
        bool expected;

        ServerConfig()
          : focus_selector(std::make_shared<mtd::MockInputFocusSelector>()),
            expected(false)
        {
        }

        std::shared_ptr<msh::InputFocusSelector>
        the_input_focus_selector() override
        {
            using namespace ::testing;

            if (!expected)
            {
                InSequence seq;

                EXPECT_CALL(*focus_selector, set_input_focus_to(NonNullSessionTarget(), NullSurfaceTarget())).Times(1);
                EXPECT_CALL(*focus_selector, set_input_focus_to(NonNullSessionTarget(), NonNullSurfaceTarget())).Times(1);
                expected = true;
            }

            return focus_selector;
        }
    } server_config;


    launch_server_process(server_config);

    SurfaceCreatingClient client;

    launch_client_process(client);
}
