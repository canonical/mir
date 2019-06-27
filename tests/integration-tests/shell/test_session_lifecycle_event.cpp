/*
 * Copyright © 2013 Canonical Ltd.
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
 *
 * Authored by: Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */

#include "mir/scene/null_session_listener.h"
#include "src/server/scene/application_session.h"

#include "mir/test/signal_actions.h"
#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"
#include "mir_test_framework/in_process_server.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <chrono>

namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

using namespace std::chrono_literals;

namespace
{

struct MockStateHandler
{
    MOCK_METHOD1(state_changed, void (MirLifecycleState));
};

void lifecycle_callback(MirConnection* /*connection*/, MirLifecycleState state, void* context)
{
    auto const handler = static_cast<MockStateHandler*>(context);
    handler->state_changed(state);
}

class StubSessionListener : public ms::NullSessionListener
{
    void stopping(std::shared_ptr<ms::Session> const& session)
    {
        auto const app_session = std::static_pointer_cast<ms::ApplicationSession>(session);

        app_session->set_lifecycle_state(mir_lifecycle_state_will_suspend);
    }
};

struct StubServerConfig : mtf::StubbedServerConfiguration
{
    std::shared_ptr<ms::SessionListener> the_session_listener() override
    {
        return std::make_shared<StubSessionListener>();
    }
};

struct LifecycleEventTest : mtf::InProcessServer
{
    StubServerConfig server_configuration;

    mir::DefaultServerConfiguration& server_config() override
        { return server_configuration; }
};

}

TEST_F(LifecycleEventTest, lifecycle_event_test)
{
    using namespace ::testing;

    auto const connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto const handler = std::make_shared<MockStateHandler>();
    mir_connection_set_lifecycle_event_callback(connection, lifecycle_callback, handler.get());

    mt::Signal signal;
    EXPECT_CALL(*handler, state_changed(Eq(mir_lifecycle_state_will_suspend))).Times(1)
        .WillOnce(mt::WakeUp(&signal));
    EXPECT_CALL(*handler, state_changed(Eq(mir_lifecycle_connection_lost))).Times(AtMost(1));

    mir_connection_release(connection);

    EXPECT_TRUE(signal.wait_for(30s));
}
