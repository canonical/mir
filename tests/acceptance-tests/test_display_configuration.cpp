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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/main_loop.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/graphics/event_handler_register.h"
#include "mir/shell/display_configuration_controller.h"

#include "mir_test_framework/connected_client_headless_server.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/test/doubles/null_display.h"
#include "mir/test/doubles/null_display_sync_group.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/test/display_config_matchers.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/stub_session_authorizer.h"
#include "mir/test/fake_shared.h"
#include "mir/test/pipe.h"
#include "mir/test/wait_condition.h"
#include "mir/test/signal.h"

#include "mir_toolkit/mir_client_library.h"

#include <atomic>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

using namespace testing;

namespace
{
mtd::StubDisplayConfig stub_display_config;

mtd::StubDisplayConfig changed_stub_display_config{1};

class MockDisplay : public mtd::NullDisplay
{
public:
    MockDisplay()
        : config{std::make_shared<mtd::StubDisplayConfig>()},
          handler_called{false}
    {
        using namespace testing;
        ON_CALL(*this, configure(_))
            .WillByDefault(Invoke(
                [this](mg::DisplayConfiguration const& new_config)
                {
                    config = std::make_shared<mtd::StubDisplayConfig>(new_config);
                }));
    }

    void for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f) override
    {
        f(display_sync_group);
    }

    std::unique_ptr<mg::DisplayConfiguration> configuration() const override
    {
        return std::unique_ptr<mg::DisplayConfiguration>(
            new mtd::StubDisplayConfig(*config)
        );
    }

    void register_configuration_change_handler(
        mg::EventHandlerRegister& handlers,
        mg::DisplayConfigurationChangeHandler const& handler) override
    {
        handlers.register_fd_handler(
            {p.read_fd()},
            this,
            [this, handler](int fd)
            {
                char c;
                if (read(fd, &c, 1) == 1)
                {
                    handler();
                    handler_called = true;
                }
            });
    }

    MOCK_METHOD1(configure, void(mg::DisplayConfiguration const&));

    void emit_configuration_change_event(
        std::shared_ptr<mtd::StubDisplayConfig> const& new_config)
    {
        config = new_config;
        if (write(p.write_fd(), "a", 1)) {}
    }

    void wait_for_configuration_change_handler()
    {
        while (!handler_called)
            std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }

private:
    std::shared_ptr<mg::DisplayConfiguration> config;
    mtd::NullDisplaySyncGroup display_sync_group;
    mt::Pipe p;
    std::atomic<bool> handler_called;
};

struct StubAuthorizer : mtd::StubSessionAuthorizer
{
    bool configure_display_is_allowed(mf::SessionCredentials const&) override { return result; }

    std::atomic<bool> result{true};
};

void wait_for_server_actions_to_finish(mir::ServerActionQueue& server_action_queue)
{
    mt::WaitCondition last_action_done;
    server_action_queue.enqueue(
        &last_action_done,
        [&] { last_action_done.wake_up_everyone(); });

    last_action_done.wait_for_at_most_seconds(5);
}
}

struct DisplayConfigurationTest : mtf::ConnectedClientHeadlessServer
{
    void SetUp() override
    {
        server.override_the_session_authorizer([this] { return mt::fake_shared(stub_authorizer); });
        preset_display(mt::fake_shared(mock_display));
        mtf::ConnectedClientHeadlessServer::SetUp();
    }

    testing::NiceMock<MockDisplay> mock_display;
    StubAuthorizer stub_authorizer;
};

TEST_F(DisplayConfigurationTest, display_configuration_reaches_client)
{
    auto configuration = mir_connection_create_display_config(connection);

    EXPECT_THAT(*configuration,
                mt::DisplayConfigMatches(std::cref(stub_display_config)));

    mir_display_config_destroy(configuration);
}

namespace
{
void display_change_handler(MirConnection* connection, void* context)
{
    auto configuration = mir_connection_create_display_config(connection);

    EXPECT_THAT(*configuration,
                mt::DisplayConfigMatches(std::cref(changed_stub_display_config)));
    mir_display_config_destroy(configuration);

    auto callback_called = static_cast<std::atomic<bool>*>(context);
    *callback_called = true;
}
}

TEST_F(DisplayConfigurationTest, hw_display_change_notification_reaches_all_clients)
{
    std::atomic<bool> callback_called{false};

    mir_connection_set_display_config_change_callback(connection, &display_change_handler, &callback_called);

    MirConnection* unsubscribed_connection = mir_connect_sync(new_connection().c_str(), "notifier");

    mock_display.emit_configuration_change_event(
        mt::fake_shared(changed_stub_display_config));

    while (!callback_called)
        std::this_thread::sleep_for(std::chrono::microseconds{500});

    // At this point, the message has gone out on the wire. since with unsubscribed_connection
    // we're emulating a client that is passively subscribed, we will just wait for the display
    // configuration to change and then will check the new config.

    auto configuration = mir_connection_create_display_config(unsubscribed_connection);
    while(configuration->num_outputs != changed_stub_display_config.outputs.size())
    {
        mir_display_config_destroy(configuration);
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        configuration = mir_connection_create_display_config(unsubscribed_connection);
    }

    EXPECT_THAT(*configuration,
                mt::DisplayConfigMatches(std::cref(changed_stub_display_config)));

    mir_display_config_destroy(configuration);

    mir_connection_release(unsubscribed_connection);
}

TEST_F(DisplayConfigurationTest, display_change_request_for_unauthorized_client_fails)
{
    stub_authorizer.result = false;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto configuration = mir_connection_create_display_config(connection);

    mir_wait_for(mir_connection_apply_display_config(connection, configuration));
    EXPECT_THAT(mir_connection_get_error_message(connection),
                testing::HasSubstr("not authorized to apply display configurations"));

    mir_display_config_destroy(configuration);
    mir_connection_release(connection);
}

namespace
{
struct SimpleClient
{
    SimpleClient(std::string const& mir_test_socket) :
        mir_test_socket{mir_test_socket} {}

    void connect()
    {
        connection = mir_connect_sync(mir_test_socket.c_str(), __PRETTY_FUNCTION__);

        auto const spec = mir_connection_create_spec_for_normal_surface(connection, 100, 100, mir_pixel_format_abgr_8888);
        surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    }

    void disconnect()
    {
        mir_surface_release_sync(surface);
        mir_connection_release(connection);
    }

    std::string mir_test_socket;
    MirConnection* connection{nullptr};
    MirSurface* surface{nullptr};
};

struct DisplayClient : SimpleClient
{
    using SimpleClient::SimpleClient;

    void apply_config()
    {
        auto configuration = mir_connection_create_display_config(connection);
        mir_wait_for(mir_connection_apply_display_config(connection, configuration));
        EXPECT_STREQ("", mir_connection_get_error_message(connection));
        mir_display_config_destroy(configuration);
    }
};
}

TEST_F(DisplayConfigurationTest, changing_config_for_focused_client_configures_display)
{
    EXPECT_CALL(mock_display, configure(_)).Times(0);

    DisplayClient display_client{new_connection()};

    display_client.connect();

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(testing::_)).Times(1);

    display_client.apply_config();

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);

    display_client.disconnect();
}

TEST_F(DisplayConfigurationTest, focusing_client_with_display_config_configures_display)
{
    EXPECT_CALL(mock_display, configure(_)).Times(0);

    SimpleClient simple_client{new_connection()};

    /* Connect the simple client. After this the simple client should have the focus. */
    simple_client.connect();

    /* Apply the display config while not focused */
    auto const configuration = mir_connection_create_display_config(connection);
    mir_wait_for(mir_connection_apply_display_config(connection, configuration));
    mir_display_config_destroy(configuration);

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(testing::_)).Times(1);

    /*
     * Shut down the simple client. After this the focus should have changed to the
     * display client and its configuration should have been applied.
     */
    simple_client.disconnect();

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);
}

TEST_F(DisplayConfigurationTest, changing_focus_from_client_with_config_to_client_without_config_configures_display)
{
    DisplayClient display_client{new_connection()};
    SimpleClient simple_client{new_connection()};

    /* Connect the simple client. */
    simple_client.connect();

    /* Connect the display config client and apply a display config. */
    display_client.connect();

    wait_for_server_actions_to_finish(*server.the_main_loop());
    EXPECT_CALL(mock_display, configure(_)).Times(1);
    display_client.apply_config();
    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(testing::_)).Times(1);

    /*
     * Shut down the display client. After this the focus should have changed to the
     * simple client and the base configuration should have been applied.
     */
    display_client.disconnect();

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);

    simple_client.disconnect();
}

TEST_F(DisplayConfigurationTest, hw_display_change_doesnt_apply_base_config_if_per_session_config_is_active)
{
    DisplayClient display_client{new_connection()};

    display_client.connect();
    display_client.apply_config();

    wait_for_server_actions_to_finish(*server.the_main_loop());
    Mock::VerifyAndClearExpectations(&mock_display);
    /*
     * A client with a per-session config is active, the base configuration
     * shouldn't be applied.
     */
    EXPECT_CALL(mock_display, configure(_)).Times(0);
    mock_display.emit_configuration_change_event(
        mt::fake_shared(changed_stub_display_config));
    mock_display.wait_for_configuration_change_handler();
    wait_for_server_actions_to_finish(*server.the_main_loop());
    Mock::VerifyAndClearExpectations(&mock_display);

    display_client.disconnect();
}

namespace
{
struct DisplayConfigMatchingContext
{
    std::function<void(MirDisplayConfiguration*)> matcher;
    mt::Signal done;
};

void new_display_config_matches(MirConnection* connection, void* ctx)
{
    auto context = reinterpret_cast<DisplayConfigMatchingContext*>(ctx);

    auto config = mir_connection_create_display_config(connection);
    context->matcher(config);
    mir_display_config_destroy(config);
    context->done.raise();
}
}

TEST_F(DisplayConfigurationTest, shell_initiated_display_configuration_notifies_clients)
{
    using namespace testing;

    // Create a new client for explicit lifetime handling.
    SimpleClient client{new_connection()};

    client.connect();

    std::shared_ptr<mg::DisplayConfiguration> new_conf;
    new_conf = server.the_display()->configuration();

    new_conf->for_each_output([](mg::UserDisplayConfigurationOutput& output)
        {
            if (output.connected)
            {
                output.used = !output.used;
            }
        });

    DisplayConfigMatchingContext context;
    context.matcher = [new_conf](MirDisplayConfiguration* conf)
        {
            EXPECT_THAT(conf, mt::DisplayConfigMatches(std::cref(*new_conf)));
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    server.the_display_configuration_controller()->set_default_display_configuration(new_conf).get();

    EXPECT_THAT(
        *server.the_display()->configuration(),
        mt::DisplayConfigMatches(std::cref(*new_conf)));

    EXPECT_TRUE(context.done.wait_for(std::chrono::seconds{10}));

    client.disconnect();
}
