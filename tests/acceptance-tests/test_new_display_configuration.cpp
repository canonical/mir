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

#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/test/doubles/fake_display.h"
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
#include <chrono>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;

using namespace testing;
using namespace std::literals::chrono_literals;

namespace
{
mtd::StubDisplayConfig stub_display_config;

mtd::StubDisplayConfig changed_stub_display_config{1};

class MockDisplay : public mtd::FakeDisplay
{
public:
    MockDisplay(): mtd::FakeDisplay()
    {
        using namespace testing;
        ON_CALL(*this, configure(_))
            .WillByDefault(Invoke(
                [this](mg::DisplayConfiguration const& new_config)
                {
                    mtd::FakeDisplay::configure(new_config);
                }));
    }

    MOCK_METHOD1(configure, void(mg::DisplayConfiguration const&));
};

struct StubAuthorizer : mtd::StubSessionAuthorizer
{
    bool configure_display_is_allowed(mf::SessionCredentials const&) override
    {
        return allow_configure_display;
    }

    bool set_base_display_configuration_is_allowed(mf::SessionCredentials const&) override
    {
        return allow_set_base_display_configuration;
    }

    std::atomic<bool> allow_configure_display{true};
    std::atomic<bool> allow_set_base_display_configuration{true};
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

struct DisplayConfigurationTest : mtf::ConnectedClientWithASurface
{
    void SetUp() override
    {
        server.override_the_session_authorizer([this] { return mt::fake_shared(stub_authorizer); });
        preset_display(mt::fake_shared(mock_display));
        mtf::ConnectedClientWithASurface::SetUp();
    }

    testing::NiceMock<MockDisplay> mock_display;
    StubAuthorizer stub_authorizer;
};

TEST_F(DisplayConfigurationTest, display_configuration_reaches_client)
{
    auto configuration = mir_connection_create_display_configuration(connection);

    EXPECT_THAT(configuration,
        mt::DisplayConfigMatches(std::cref(stub_display_config)));

    mir_display_config_release(configuration);
}

namespace
{
void display_change_handler(MirConnection* connection, void* context)
{
    auto configuration = mir_connection_create_display_configuration(connection);

    EXPECT_THAT(configuration,
                mt::DisplayConfigMatches(std::cref(changed_stub_display_config)));
    mir_display_config_release(configuration);

    auto callback_called = static_cast<mt::Signal*>(context);
    callback_called->raise();
}
}

TEST_F(DisplayConfigurationTest, hw_display_change_notification_reaches_all_clients)
{
    mt::Signal callback_called;

    mir_connection_set_display_config_change_callback(connection, &display_change_handler, &callback_called);

    MirConnection* unsubscribed_connection = mir_connect_sync(new_connection().c_str(), "notifier");

    mock_display.emit_configuration_change_event(
        mt::fake_shared(changed_stub_display_config));

    EXPECT_TRUE(callback_called.wait_for(std::chrono::seconds{10}));

    // At this point, the message has gone out on the wire. since with unsubscribed_connection
    // we're emulating a client that is passively subscribed, we will just wait for the display
    // configuration to change and then will check the new config.

    auto config = mir_connection_create_display_configuration(unsubscribed_connection);
    while((unsigned)mir_display_config_get_num_outputs(config) != changed_stub_display_config.outputs.size())
    {
        mir_display_config_release(config);
        std::this_thread::sleep_for(std::chrono::microseconds(500));
        config = mir_connection_create_display_configuration(unsubscribed_connection);
    }

    EXPECT_THAT(config,
        mt::DisplayConfigMatches(std::cref(changed_stub_display_config)));

    mir_display_config_release(config);

    mir_connection_release(unsubscribed_connection);
}

TEST_F(DisplayConfigurationTest, display_change_request_for_unauthorized_client_fails)
{
    stub_authorizer.allow_configure_display = false;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto configuration = mir_connection_create_display_configuration(connection);

    mir_wait_for(mir_connection_apply_display_configuration(connection, configuration));
    EXPECT_THAT(mir_connection_get_error_message(connection),
        testing::HasSubstr("not authorized to apply display configurations"));

    mir_display_config_release(configuration);
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

    void disconnect_without_releasing_surface()
    {
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
        auto configuration = mir_connection_create_display_configuration(connection);
        mir_wait_for(mir_connection_apply_display_configuration(connection, configuration));
        EXPECT_STREQ("", mir_connection_get_error_message(connection));
        mir_display_config_release(configuration);
    }

    std::unique_ptr<MirDisplayConfig,void(*)(MirDisplayConfig*)> get_base_config()
    {
        return {mir_connection_create_display_configuration(connection),
            &mir_display_config_release};
    }

    void set_base_config(MirDisplayConfig* configuration)
    {
        mir_wait_for(mir_connection_set_base_display_configuration(connection, configuration));
        EXPECT_STREQ("", mir_connection_get_error_message(connection));
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
    auto const configuration = mir_connection_create_display_configuration(connection);
    mir_wait_for(mir_connection_apply_display_configuration(connection, configuration));
    mir_display_config_release(configuration);

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
    mock_display.emit_configuration_change_event(mt::fake_shared(changed_stub_display_config));
    mock_display.wait_for_configuration_change_handler();
    wait_for_server_actions_to_finish(*server.the_main_loop());
    Mock::VerifyAndClearExpectations(&mock_display);

    display_client.disconnect();
}

namespace
{
struct DisplayConfigMatchingContext
{
    std::function<void(MirDisplayConfig*)> matcher;
    mt::Signal done;
};

void new_display_config_matches(MirConnection* connection, void* ctx)
{
    auto context = reinterpret_cast<DisplayConfigMatchingContext*>(ctx);

    auto config = mir_connection_create_display_configuration(connection);
    context->matcher(config);
    mir_display_config_release(config);
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
    context.matcher = [new_conf](MirDisplayConfig* conf)
    {
        EXPECT_THAT(conf, mt::DisplayConfigMatches(std::cref(*new_conf)));
    };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    server.the_display_configuration_controller()->set_base_configuration(new_conf);

    EXPECT_TRUE(context.done.wait_for(std::chrono::seconds{10}));

    EXPECT_THAT(
        *server.the_display()->configuration(),
        mt::DisplayConfigMatches(std::cref(*new_conf)));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest,
    client_setting_base_config_configures_display_if_a_session_config_is_not_applied)
{
    DisplayClient display_client{new_connection()};
    display_client.connect();

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(testing::_)).Times(1);

    display_client.set_base_config(display_client.get_base_config().get());

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);

    display_client.disconnect();
}

TEST_F(DisplayConfigurationTest,
    client_setting_base_config_does_not_configure_display_if_a_session_config_is_applied)
{
    DisplayClient display_client{new_connection()};
    DisplayClient display_client_with_session_config{new_connection()};
    // Client with session config should have focus after this
    display_client.connect();
    display_client_with_session_config.connect();

    display_client_with_session_config.apply_config();

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(testing::_)).Times(0);

    display_client.set_base_config(display_client.get_base_config().get());

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);

    display_client_with_session_config.disconnect();
    display_client.disconnect();
}

namespace
{
void display_config_change_handler(MirConnection*, void* context)
{
    auto callback_called = static_cast<mt::Signal*>(context);
    callback_called->raise();
}
}

TEST_F(DisplayConfigurationTest,
    client_is_notified_of_new_base_config_eventually_after_set_base_configuration)
{
    DisplayClient display_client{new_connection()};
    display_client.connect();

    mt::Signal callback_called;
    mir_connection_set_display_config_change_callback(
        display_client.connection, &display_config_change_handler, &callback_called);

    auto requested_config = display_client.get_base_config();
    auto output = mir_display_config_get_mutable_output(requested_config.get(), 0);
    EXPECT_TRUE(mir_output_is_enabled(output));

    mir_output_disable(output);

    display_client.set_base_config(requested_config.get());

    EXPECT_THAT(callback_called.wait_for(5s), Eq(true));

    auto const new_config = display_client.get_base_config();
    EXPECT_THAT(new_config.get(), mt::DisplayConfigMatches(requested_config.get()));

    display_client.disconnect();
}

TEST_F(DisplayConfigurationTest,
    set_base_configuration_for_unauthorized_client_fails)
{
    stub_authorizer.allow_set_base_display_configuration = false;

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto configuration = mir_connection_create_display_configuration(connection);
    mir_wait_for(mir_connection_set_base_display_configuration(connection, configuration));
    EXPECT_THAT(mir_connection_get_error_message(connection),
        testing::HasSubstr("not authorized to set base display configuration"));

    mir_display_config_release(configuration);
    mir_connection_release(connection);
}

TEST_F(DisplayConfigurationTest, disconnection_of_client_with_display_config_reconfigures_display)
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

    EXPECT_CALL(mock_display, configure(_)).Times(1);

    display_client.disconnect();

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);
}

TEST_F(DisplayConfigurationTest,
    disconnection_without_releasing_surfaces_of_client_with_display_config_reconfigures_display)
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

    EXPECT_CALL(mock_display, configure(_)).Times(1);

    display_client.disconnect_without_releasing_surface();

    wait_for_server_actions_to_finish(*server.the_main_loop());
    testing::Mock::VerifyAndClearExpectations(&mock_display);
}


struct DisplayPowerSetting : public DisplayConfigurationTest, public ::testing::WithParamInterface<MirPowerMode> {};
struct DisplayOrientationSetting : public DisplayConfigurationTest, public ::testing::WithParamInterface<MirOrientation> {};
struct DisplayFormatSetting : public DisplayConfigurationTest, public ::testing::WithParamInterface<MirPixelFormat> {};

TEST_P(DisplayPowerSetting, can_set_power_mode)
{
    using namespace testing;

    auto mode = GetParam();
    DisplayClient client{new_connection()};

    client.connect();

    auto config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(config.get()); ++i)
    {
        auto output = mir_display_config_get_mutable_output(config.get(), i);

        mir_output_set_power_mode(output, mode);
    }

    EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(config.get())));
    mir_wait_for(mir_connection_apply_display_configuration(client.connection, config.get()));

    wait_for_server_actions_to_finish(*server.the_main_loop());
    Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(_)).Times(AnyNumber());
    client.disconnect();
}

TEST_P(DisplayOrientationSetting, can_set_orientation)
{
    using namespace testing;

    auto orientation = GetParam();
    DisplayClient client{new_connection()};

    client.connect();

    auto config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(config.get()); ++i)
    {
        auto output = mir_display_config_get_mutable_output(config.get(), i);

        mir_output_set_orientation(output, orientation);
    }

    EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(config.get())));
    mir_wait_for(mir_connection_apply_display_configuration(client.connection, config.get()));

    wait_for_server_actions_to_finish(*server.the_main_loop());
    Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(_)).Times(AnyNumber());
    client.disconnect();
}

namespace
{
std::vector<MirPixelFormat> const formats{
    mir_pixel_format_abgr_8888,
    mir_pixel_format_xbgr_8888,
    mir_pixel_format_argb_8888,
    mir_pixel_format_xrgb_8888,
    mir_pixel_format_bgr_888,
    mir_pixel_format_rgb_888,
    mir_pixel_format_rgb_565,
    mir_pixel_format_rgba_5551,
    mir_pixel_format_rgba_4444,
};
}

TEST_P(DisplayFormatSetting, can_set_output_format)
{
    using namespace testing;
    auto format = GetParam();

    mtd::StubDisplayConfig all_format_config(1, formats);

    mock_display.emit_configuration_change_event(mt::fake_shared(all_format_config));

    DisplayClient client{new_connection()};

    client.connect();

    auto config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(config.get()); ++i)
    {
        auto output = mir_display_config_get_mutable_output(config.get(), i);

        if (mir_output_is_enabled(output))
        {
            mir_output_set_format(output, format);
        }
    }

    EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(config.get())));
    mir_wait_for(mir_connection_apply_display_configuration(client.connection, config.get()));

    wait_for_server_actions_to_finish(*server.the_main_loop());
    Mock::VerifyAndClearExpectations(&mock_display);

    EXPECT_CALL(mock_display, configure(_)).Times(AnyNumber());
    client.disconnect();
}

INSTANTIATE_TEST_CASE_P(DisplayConfiguration, DisplayPowerSetting,
    Values(mir_power_mode_on, mir_power_mode_standby, mir_power_mode_suspend, mir_power_mode_off));

INSTANTIATE_TEST_CASE_P(DisplayConfiguration, DisplayFormatSetting,
    ValuesIn(formats));

TEST_F(DisplayConfigurationTest, can_set_display_mode)
{
    mg::DisplayConfigurationMode hd{{1280, 720}, 60.0};
    mg::DisplayConfigurationMode fhd{{1920, 1080}, 60.0};
    mg::DisplayConfigurationMode proper_size{{1920, 1200}, 60.0};
    mg::DisplayConfigurationMode retina{{3210, 2800}, 60.0};

    mtd::StubDisplayConfigurationOutput tv{
        mg::DisplayConfigurationOutputId{1},
        {hd, fhd},
        {mir_pixel_format_abgr_8888}};
    mtd::StubDisplayConfigurationOutput monitor{
        mg::DisplayConfigurationOutputId{2},
        {hd, fhd, proper_size, retina},
        {mir_pixel_format_abgr_8888}};

    std::vector<mg::DisplayConfigurationOutput> outputs{tv, monitor};

    mock_display.emit_configuration_change_event(std::make_shared<mtd::StubDisplayConfig>(outputs));

    DisplayClient client{new_connection()};

    client.connect();

    for (size_t tv_index = 0; tv_index < tv.modes.size(); ++tv_index)
    {
        for (size_t monitor_index = 0; monitor_index < monitor.modes.size(); ++monitor_index)
        {
            auto config = client.get_base_config();

            for (int i = 0; i < mir_display_config_get_num_outputs(config.get()); ++i)
            {
                auto output = mir_display_config_get_mutable_output(config.get(), i);

                if (mir_output_get_num_modes(output) == static_cast<int>(monitor.modes.size()))
                {
                    mir_output_set_current_mode(output, mir_output_get_mode(output, monitor_index));
                }
                else if (mir_output_get_num_modes(output) == static_cast<int>(tv.modes.size()))
                {
                    mir_output_set_current_mode(output, mir_output_get_mode(output, tv_index));
                }
                else
                {
                    FAIL() << "Unexpected output in configuration";
                }
            }

            EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(config.get())));
            mir_wait_for(mir_connection_apply_display_configuration(client.connection, config.get()));

            wait_for_server_actions_to_finish(*server.the_main_loop());
            Mock::VerifyAndClearExpectations(&mock_display);
        }
    }

    EXPECT_CALL(mock_display, configure(_)).Times(AnyNumber());
    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_receives_correct_mode_information)
{
    mg::DisplayConfigurationMode hd{{1280, 720}, 60.0};
    mg::DisplayConfigurationMode fhd{{1920, 1080}, 60.0};
    mg::DisplayConfigurationMode proper_size{{1920, 1200}, 60.0};
    mg::DisplayConfigurationMode retina{{3210, 2800}, 60.0};

    mg::DisplayConfigurationOutputId const id{2};

    std::vector<mg::DisplayConfigurationMode> modes{hd, fhd, proper_size, retina};

    mtd::StubDisplayConfigurationOutput monitor{
        id,
        modes,
        {mir_pixel_format_abgr_8888}};

    std::vector<mg::DisplayConfigurationOutput> outputs{monitor};

    mock_display.emit_configuration_change_event(std::make_shared<mtd::StubDisplayConfig>(outputs));

    DisplayClient client{new_connection()};

    client.connect();

    auto config = client.get_base_config();

    ASSERT_THAT(mir_display_config_get_num_outputs(config.get()), Eq(1));

    std::vector<mg::DisplayConfigurationMode> received_modes;

    auto output = mir_display_config_get_output(config.get(), 0);

    for (auto i = 0; i < mir_output_get_num_modes(output) ; ++i)
    {
        auto mode = mir_output_get_mode(output, i);
        auto width = mir_output_mode_get_width(mode);
        auto height = mir_output_mode_get_height(mode);
        auto refresh = mir_output_mode_get_refresh_rate(mode);

        received_modes.push_back(mg::DisplayConfigurationMode{{width, height}, refresh});
    }

    EXPECT_THAT(received_modes, ContainerEq(modes));

    client.disconnect();
}
