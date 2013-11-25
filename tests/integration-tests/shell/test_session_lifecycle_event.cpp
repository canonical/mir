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
 * Authored by: Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */

#include "mir/shell/null_session_listener.h"
#include "src/server/scene/application_session.h"

#include "mir_test_framework/display_server_test_fixture.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <unistd.h>
#include <fcntl.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mtf = mir_test_framework;

namespace
{
char const* const mir_test_socket = mtf::test_socket_file().c_str();

struct MockStateHandler
{
    MOCK_METHOD1(state_changed, void (MirLifecycleState));
};

class StubSessionListener : public msh::NullSessionListener
{
    void stopping(std::shared_ptr<msh::Session> const& session)
    {
        std::shared_ptr<ms::ApplicationSession> app_session(
            std::static_pointer_cast<ms::ApplicationSession>(session)
            );

        app_session->set_lifecycle_state(mir_lifecycle_state_will_suspend);
    }
};

using LifecycleEventTest = BespokeDisplayServerTestFixture;
TEST_F(LifecycleEventTest, lifecycle_event_test)
{
    using namespace ::testing;

    struct ServerConfig : TestingServerConfiguration
    {
        std::shared_ptr<msh::SessionListener> the_shell_session_listener() override
        {
            return std::make_shared<StubSessionListener>();
        }
    } server_config;

    launch_server_process(server_config);

    struct ClientConfig : TestingClientConfiguration
    {
        static void lifecycle_callback(MirConnection* /*connection*/, MirLifecycleState state, void* context)
        {
            auto config = static_cast<ClientConfig*>(context);
            config->handler->state_changed(state);
        }

        void exec()
        {
            handler = std::make_shared<MockStateHandler>();

            MirConnection* connection = mir_connect_sync(mir_test_socket, "testapp");
            mir_connection_set_lifecycle_event_callback(connection, lifecycle_callback, this);

            EXPECT_CALL(*handler, state_changed(Eq(mir_lifecycle_state_will_suspend))).Times(1);
            mir_connection_release(connection);

            handler.reset();
        }

        std::shared_ptr<MockStateHandler> handler;
    } client_config;

    launch_client_process(client_config);
}
}
