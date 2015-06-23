/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir/test/wait_object.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/null_display_changer.h"

#include "mir_toolkit/mir_connection.h"

#include <gtest/gtest.h>

namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mg = mir::graphics;
namespace mf = mir::frontend;

namespace
{

class LargeDisplayConfigChanger : public mtd::NullDisplayChanger
{
public:
    std::shared_ptr<mg::DisplayConfiguration> active_configuration() override
    {
        return mt::fake_shared(stub_display_config);
    }

private:
    mtd::StubDisplayConfig stub_display_config{20};
};


struct LargeMessagesServerConfiguration : mir_test_framework::StubbedServerConfiguration
{
    std::shared_ptr<mf::DisplayChanger> the_frontend_display_changer() override
    {
        return mt::fake_shared(large_display_config_changer);
    }

    LargeDisplayConfigChanger large_display_config_changer;
};

struct LargeMessages : mir_test_framework::InProcessServer
{
    LargeMessagesServerConfiguration large_messages_server_config;

    mir::DefaultServerConfiguration& server_config() override
    {
        return large_messages_server_config;
    }

    mir_test_framework::UsingStubClientPlatform using_stub_client_platform;
};

struct ConnectionContext
{
    mt::WaitObject connected;
    MirConnection* connection{nullptr};
};

void connection_callback(MirConnection* connection, void* context)
{
    auto connection_context = static_cast<ConnectionContext*>(context);
    connection_context->connection = connection;
    connection_context->connected.notify_ready();
}

}

// Regression test for lp:1320187. Without a fix for that bug, connecting
// to a server with a lot of information in the display configuration hangs
// the client, because the server reply message is incomplete/corrupted.
TEST_F(LargeMessages, connection_to_server_with_large_display_configuration_succeeds)
{
    using namespace testing;

    ConnectionContext connection_context;

    mir_connect(new_connection().c_str(), __PRETTY_FUNCTION__,
                connection_callback, &connection_context);

    connection_context.connected.wait_until_ready(std::chrono::seconds{3});

    EXPECT_NE(connection_context.connection, nullptr);

    mir_connection_release(connection_context.connection);
}
