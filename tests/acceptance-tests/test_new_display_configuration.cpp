/*
 * Copyright © 2013-2016 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 *              Kevin DuBois <kevin.dubois@canonical.com>
 *              Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/main_loop.h"
#include "mir/frontend/session_authorizer.h"
#include "mir/graphics/event_handler_register.h"
#include "mir/shell/display_configuration_controller.h"
#include "mir/graphics/display_configuration_observer.h"
#include "mir/observer_registrar.h"

#include "mir_test_framework/connected_client_with_a_window.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/test/doubles/fake_display.h"
#include "mir/test/doubles/null_display_sync_group.h"
#include "mir/test/doubles/null_platform.h"
#include "mir/test/display_config_matchers.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/stub_session_authorizer.h"
#include "mir/test/fake_shared.h"
#include "mir/test/pipe.h"
#include "mir/test/signal_actions.h"

#include "mir_toolkit/mir_client_library.h"

#include <atomic>
#include <chrono>
#include <mutex>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <future>

namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace ms = mir::scene;
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
}

struct DisplayConfigurationTest : mtf::ConnectedClientWithAWindow
{
    class NotifyingConfigurationObserver : public mg::DisplayConfigurationObserver
    {
    public:
        std::future<void> expect_configuration_change(
            std::shared_ptr<mg::DisplayConfiguration const> const& configuration)
        {
            std::lock_guard<decltype(guard)> lock{guard};
            expectations.push_back({{}, configuration});
            return expectations.back().notifier.get_future();
        }

        MOCK_METHOD2(session_configuration_applied, void(
            std::shared_ptr<ms::Session> const&,
            std::shared_ptr<mg::DisplayConfiguration> const&));
        MOCK_METHOD1(session_configuration_removed, void(std::shared_ptr<ms::Session> const&));

    protected:
        void initial_configuration(
            std::shared_ptr<mg::DisplayConfiguration const> const&) override
        {
        }

        void configuration_applied(
            std::shared_ptr<mg::DisplayConfiguration const> const& config) override
        {
            std::lock_guard<decltype(guard)> lock{guard};
            auto match = std::find_if(
                expectations.begin(),
                expectations.end(),
                [config](auto const& candidate)
                {
                    return *config == *candidate.value;
                });

            if (match != expectations.end())
            {
                match->notifier.set_value();
                expectations.erase(match);
            }
        }

        void base_configuration_updated(
            std::shared_ptr<mg::DisplayConfiguration const> const&) override
        {
        }

        void configuration_failed(
            std::shared_ptr<mg::DisplayConfiguration const> const&,
            std::exception const&) override
        {
        }

        void catastrophic_configuration_error(
            std::shared_ptr<mg::DisplayConfiguration const> const&,
            std::exception const&) override
        {
        }

    private:
        struct Expectation
        {
            std::promise<void> notifier;
            std::shared_ptr<mg::DisplayConfiguration const> value;
        };

        std::mutex mutable guard;
        std::vector<Expectation> expectations;
    };

    void SetUp() override
    {
        server.override_the_session_authorizer([this] { return mt::fake_shared(stub_authorizer); });
        preset_display(mt::fake_shared(mock_display));
        mtf::ConnectedClientWithAWindow::SetUp();

        server.the_display_configuration_observer_registrar()->register_interest(observer);
    }

    void apply_config_change_and_wait_for_propagation(
        std::shared_ptr<mg::DisplayConfiguration> const& new_config)
    {
        auto future = observer->expect_configuration_change(new_config);

        server.the_display_configuration_controller()->set_base_configuration(new_config);

        if (future.wait_for(std::chrono::seconds{10}) != std::future_status::ready)
        {
            BOOST_THROW_EXCEPTION(std::runtime_error{"Timeout waiting for display configuration to apply"});
        }
    }

    testing::NiceMock<MockDisplay> mock_display;
    std::shared_ptr<NotifyingConfigurationObserver> observer{std::make_shared<NotifyingConfigurationObserver>()};
    StubAuthorizer stub_authorizer;
    mir::test::Signal observed_changed;
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

namespace
{
struct SimpleClient
{
    SimpleClient(std::string const& mir_test_socket) :
        mir_test_socket{mir_test_socket} {}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    void connect()
    {
        connection = mir_connect_sync(mir_test_socket.c_str(), __PRETTY_FUNCTION__);

        auto const spec = mir_create_normal_window_spec(connection, 100, 100);
        mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
        mir_window_spec_set_event_handler(spec, &handle_event, this);
        window = mir_create_window_sync(spec);
        mir_window_spec_release(spec);
        mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));

        ready_to_accept_events.wait_for(4s);
        if (!ready_to_accept_events.raised())
            BOOST_THROW_EXCEPTION(std::runtime_error("Timeout waiting for window to become focused and exposed"));
    }
#pragma GCC diagnostic pop
    static void handle_event(MirWindow*, MirEvent const* ev, void* context)
    {
        auto const client = static_cast<SimpleClient*>(context);
        auto type = mir_event_get_type(ev);
        if (type == mir_event_type_window)
        {
            auto window_event = mir_event_get_window_event(ev);
            auto const attrib  = mir_window_event_get_attribute(window_event);
            auto const value   = mir_window_event_get_attribute_value(window_event);

            std::lock_guard<std::mutex> lk(client->mutex);
            if (mir_window_attrib_focus == attrib &&
                mir_window_focus_state_focused == value)
                client->ready_to_accept_events.raise();
        }
    }

    void disconnect()
    {
        mir_window_release_sync(window);
        mir_connection_release(connection);
    }

    void disconnect_without_releasing_surface()
    {
        mir_connection_release(connection);
    }

    std::string mir_test_socket;
    MirConnection* connection{nullptr};
    MirWindow* window{nullptr};
    mutable std::mutex mutex;
    mir::test::Signal ready_to_accept_events;
    bool focused{false};
};

struct DisplayClient : SimpleClient
{
    using SimpleClient::SimpleClient;

    std::unique_ptr<MirDisplayConfig,void(*)(MirDisplayConfig*)> get_base_config()
    {
        return {mir_connection_create_display_configuration(connection),
            &mir_display_config_release};
    }
};
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

struct DisplayPowerSetting : public DisplayConfigurationTest, public ::testing::WithParamInterface<MirPowerMode> {};
struct DisplayOrientationSetting : public DisplayConfigurationTest, public ::testing::WithParamInterface<MirOrientation> {};
struct DisplayFormatSetting : public DisplayConfigurationTest, public ::testing::WithParamInterface<MirPixelFormat> {};
struct DisplaySubpixelSetting : public DisplayConfigurationTest, public ::testing::WithParamInterface<MirSubpixelArrangement> {};

TEST_P(DisplayPowerSetting, can_get_power_mode)
{
    using namespace testing;

    auto mode = GetParam();

    std::shared_ptr<mg::DisplayConfiguration> server_config = server.the_display()->configuration();

    server_config->for_each_output(
        [mode](mg::UserDisplayConfigurationOutput& output)
        {
            output.power_mode = mode;
        });

    apply_config_change_and_wait_for_propagation(server_config);

    DisplayClient client{new_connection()};

    client.connect();

    auto client_config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_output(client_config.get(), i);

        EXPECT_THAT(mir_output_get_power_mode(output), Eq(mode));
    }

    client.disconnect();
}

TEST_P(DisplayOrientationSetting, can_get_orientation)
{
    using namespace testing;

    auto orientation = GetParam();

    std::shared_ptr<mg::DisplayConfiguration> server_config = server.the_display()->configuration();

    server_config->for_each_output(
        [orientation](mg::UserDisplayConfigurationOutput& output)
        {
            output.orientation = orientation;
        });

    apply_config_change_and_wait_for_propagation(server_config);

    DisplayClient client{new_connection()};

    client.connect();

    auto client_config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_output(client_config.get(), i);

        EXPECT_THAT(mir_output_get_orientation(output), Eq(orientation));
    }

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

TEST_P(DisplayFormatSetting, can_get_all_output_format)
{
    using namespace testing;

    auto format = GetParam();
    auto single_format_config =
        std::make_shared<mtd::StubDisplayConfig>(1, std::vector<MirPixelFormat>{format});

    apply_config_change_and_wait_for_propagation(single_format_config);

    DisplayClient client{new_connection()};

    client.connect();

    auto client_config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_output(client_config.get(), i);

        EXPECT_THAT(mir_output_get_current_pixel_format(output), Eq(format));
    }

    client.disconnect();
}

TEST_P(DisplaySubpixelSetting, can_get_all_subpixel_arrangements)
{
    using namespace testing;

    auto subpixel_arrangement = GetParam();
    mtd::StubDisplayConfigurationOutput output{
        {1920, 1200},
        {200, 100},
        mir_pixel_format_abgr_8888,
        60.0,
        true,
        subpixel_arrangement};

    auto single_subpixel_config =
        std::make_shared<mtd::StubDisplayConfig>(std::vector<mg::DisplayConfigurationOutput>{output});

    apply_config_change_and_wait_for_propagation(single_subpixel_config);

    DisplayClient client{new_connection()};

    client.connect();

    auto client_config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_output(client_config.get(), i);

        EXPECT_THAT(mir_output_get_subpixel_arrangement(output), Eq(subpixel_arrangement));
    }

    client.disconnect();
}


INSTANTIATE_TEST_CASE_P(DisplayConfiguration, DisplayPowerSetting,
    Values(mir_power_mode_on, mir_power_mode_standby, mir_power_mode_suspend, mir_power_mode_off));

INSTANTIATE_TEST_CASE_P(DisplayConfiguration, DisplayFormatSetting,
    ValuesIn(formats));

INSTANTIATE_TEST_CASE_P(DisplayConfiguration, DisplaySubpixelSetting,
    Values(
        mir_subpixel_arrangement_unknown,
        mir_subpixel_arrangement_horizontal_rgb,
        mir_subpixel_arrangement_horizontal_bgr,
        mir_subpixel_arrangement_vertical_rgb,
        mir_subpixel_arrangement_vertical_bgr,
        mir_subpixel_arrangement_none));

TEST_F(DisplayConfigurationTest, client_received_configuration_matches_server_config)
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

    auto config = std::make_shared<mtd::StubDisplayConfig>(outputs);

    apply_config_change_and_wait_for_propagation(config);

    DisplayClient client{new_connection()};

    client.connect();

    auto client_config = client.get_base_config();
    auto server_config = server.the_display()->configuration();

    EXPECT_THAT(client_config.get(), mt::DisplayConfigMatches(std::cref(*server_config)));

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

    apply_config_change_and_wait_for_propagation(std::make_shared<mtd::StubDisplayConfig>(outputs));

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

TEST_F(DisplayConfigurationTest, mode_width_and_height_are_independent_of_orientation)
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

    std::shared_ptr<mg::DisplayConfiguration> server_config;
    server_config = std::make_shared<mtd::StubDisplayConfig>(outputs);
    apply_config_change_and_wait_for_propagation(server_config);

    DisplayClient client{new_connection()};
    client.connect();


    DisplayConfigMatchingContext context;
    context.matcher = [&server_config](MirDisplayConfig* conf)
        {
            EXPECT_THAT(conf, mt::DisplayConfigMatches(std::cref(*server_config)));
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    for (auto const orientation :
        {mir_orientation_normal, mir_orientation_left, mir_orientation_inverted, mir_orientation_right})
    {
        server_config = server.the_display()->configuration();
        server_config->for_each_output(
            [orientation](mg::UserDisplayConfigurationOutput& output)
            {
                output.orientation = orientation;
            });
        server.the_display_configuration_controller()->set_base_configuration(server_config);

        EXPECT_TRUE(context.done.wait_for(std::chrono::seconds{10}));
        context.done.reset();

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

        // But logical size is affected by orientation:
        auto current_mode = mir_output_get_current_mode(output);
        ASSERT_TRUE(current_mode);
        unsigned physical_width = mir_output_mode_get_width(current_mode);
        unsigned physical_height = mir_output_mode_get_height(current_mode);
        unsigned logical_width = mir_output_get_logical_width(output);
        unsigned logical_height = mir_output_get_logical_height(output);

        switch (orientation)
        {
        case mir_orientation_normal:
        case mir_orientation_inverted:
            EXPECT_EQ(physical_width, logical_width);
            EXPECT_EQ(physical_height, logical_height);
            break;
        case mir_orientation_left:
        case mir_orientation_right:
            EXPECT_EQ(physical_height, logical_width);
            EXPECT_EQ(physical_width, logical_height);
            break;
        default:
            break;
        }
    }

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, output_position_is_independent_of_orientation)
{
    std::array<mir::geometry::Point, 3> const positions = {{
        mir::geometry::Point{-100, 10},
        mir::geometry::Point{100, 10000},
        mir::geometry::Point{-100, 10}
    }};

    std::shared_ptr<mg::DisplayConfiguration> server_config = server.the_display()->configuration();
    server_config->for_each_output(
        [position = positions.begin()](mg::UserDisplayConfigurationOutput& output) mutable
        {
            output.top_left = *position;
            ++position;
        });

    DisplayClient client{new_connection()};

    client.connect();

    DisplayConfigMatchingContext context;
    context.matcher = [&server_config](MirDisplayConfig* conf)
        {
            EXPECT_THAT(conf, mt::DisplayConfigMatches(std::cref(*server_config)));
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    server.the_display_configuration_controller()->set_base_configuration(server_config);
    context.done.wait_for(std::chrono::seconds{10});

    for (auto const orientation :
        {mir_orientation_normal, mir_orientation_left, mir_orientation_inverted, mir_orientation_right})
    {
        server_config = server.the_display()->configuration();
        server_config->for_each_output(
            [orientation](mg::UserDisplayConfigurationOutput& output)
                {
                    output.orientation = orientation;
                });

        context.done.reset();
        server.the_display_configuration_controller()->set_base_configuration(server_config);

        EXPECT_TRUE(context.done.wait_for(std::chrono::seconds{10}));

        auto config = client.get_base_config();

        auto position = positions.begin();
        for (auto i = 0; i < mir_display_config_get_num_outputs(config.get()); ++i)
        {
            auto output = mir_display_config_get_output(config.get(), i);

            EXPECT_THAT(mir_output_get_position_x(output), Eq(position->x.as_int()));
            EXPECT_THAT(mir_output_get_position_y(output), Eq(position->y.as_int()));

            ++position;
        }
    }

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_receives_correct_output_positions)
{
    std::array<mir::geometry::Point, 3> const positions = {{
        mir::geometry::Point{-100, 10},
        mir::geometry::Point{100, 10000},
        mir::geometry::Point{-100, 10}
    }};

    std::shared_ptr<mg::DisplayConfiguration> server_config = server.the_display()->configuration();
    server_config->for_each_output(
    [position = positions.begin()](mg::UserDisplayConfigurationOutput& output) mutable
    {
        output.top_left = *position;
        ++position;
    });

    DisplayClient client{new_connection()};

    client.connect();

    DisplayConfigMatchingContext context;
    context.matcher = [server_config](MirDisplayConfig* conf)
        {
            EXPECT_THAT(conf, mt::DisplayConfigMatches(std::cref(*server_config)));
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    server.the_display_configuration_controller()->set_base_configuration(server_config);

    EXPECT_TRUE(context.done.wait_for(std::chrono::seconds{10}));

    auto config = client.get_base_config();

    auto position = positions.begin();
    for (auto i = 0; i < mir_display_config_get_num_outputs(config.get()); ++i)
    {
        auto output = mir_display_config_get_output(config.get(), i);

        EXPECT_THAT(mir_output_get_position_x(output), Eq(position->x.as_int()));
        EXPECT_THAT(mir_output_get_position_y(output), Eq(position->y.as_int()));

        ++position;
    }

    client.disconnect();
}

namespace
{
void signal_when_config_received(MirConnection* /*unused*/, void* ctx)
{
    auto signal = reinterpret_cast<mt::Signal*>(ctx);

    signal->raise();
}
}

TEST_F(DisplayConfigurationTest, client_sees_server_set_scale_factor)
{
    std::shared_ptr<mg::DisplayConfiguration> current_config = server.the_display()->configuration();
    current_config->for_each_output(
        [output_num = 0](mg::UserDisplayConfigurationOutput& output) mutable
        {
            output.scale = 1 + 0.25f * output_num;
            ++output_num;
        });

    DisplayClient client{new_connection()};

    client.connect();

    mt::Signal configuration_received;
    mir_connection_set_display_config_change_callback(
        client.connection,
        &signal_when_config_received,
        &configuration_received);

    server.the_display_configuration_controller()->set_base_configuration(current_config);

    EXPECT_TRUE(configuration_received.wait_for(std::chrono::seconds{10}));

    auto client_config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_output(client_config.get(), i);

        EXPECT_THAT(mir_output_get_scale_factor(output), Eq(1 + 0.25 * i));
    }

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_can_set_scale_factor)
{
    DisplayClient client{new_connection()};

    client.connect();

    auto client_config = client.get_base_config();
    auto const scale_factor = 1.5f;

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_mutable_output(client_config.get(), i);

        mir_output_set_scale_factor(output, scale_factor + 0.25f*i);
    }

    DisplayConfigMatchingContext context;
    context.matcher = [c = client_config.get()](MirDisplayConfig* conf)
        {
            EXPECT_THAT(conf, mt::DisplayConfigMatches(c));
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    mir_connection_preview_base_display_configuration(client.connection, client_config.get(), 10);

    EXPECT_TRUE(context.done.wait_for(std::chrono::seconds{5}));

    mir_connection_confirm_base_display_configuration(client.connection, client_config.get());

    std::shared_ptr<mg::DisplayConfiguration> current_config = server.the_display()->configuration();
    current_config->for_each_output(
        [scale_factor, output_num = 0](mg::UserDisplayConfigurationOutput& output) mutable
        {
            EXPECT_THAT(output.scale, Eq(scale_factor + output_num * 0.25f));
            ++output_num;
        });

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_can_set_logical_size)
{
    DisplayClient client{new_connection()};

    client.connect();

    auto client_config = client.get_base_config();
    int num_outputs = mir_display_config_get_num_outputs(client_config.get());

    for (int i = 0; i < num_outputs; ++i)
    {
        auto output =
            mir_display_config_get_mutable_output(client_config.get(), i);
        EXPECT_FALSE(mir_output_has_custom_logical_size(output));
        mir_output_set_logical_size(output, (i+1)*123, (i+3)*345);
        ASSERT_TRUE(mir_output_has_custom_logical_size(output));
    }

    DisplayConfigMatchingContext context;
    context.matcher = [c = client_config.get()](MirDisplayConfig* conf)
        {
            EXPECT_THAT(conf, mt::DisplayConfigMatches(c));
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    mir_connection_preview_base_display_configuration(client.connection,
                                                      client_config.get(), 10);

    EXPECT_TRUE(context.done.wait_for(std::chrono::seconds(30)));

    mir_connection_confirm_base_display_configuration(client.connection,
                                                      client_config.get());

    std::shared_ptr<mg::DisplayConfiguration> current_config =
        server.the_display()->configuration();

    int j = 0;
    current_config->for_each_output(
        [&j](mg::UserDisplayConfigurationOutput& output) mutable
        {
            ASSERT_TRUE(output.custom_logical_size.is_set());
            auto const& size = output.custom_logical_size.value();
            int w = size.width.as_int();
            int h = size.height.as_int();
            EXPECT_EQ((j+1)*123, w);
            EXPECT_EQ((j+3)*345, h);
            ++j;
        });

    EXPECT_TRUE(j);

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_sees_server_set_form_factor)
{
    std::array<MirFormFactor, 3> const form_factors = {{
        mir_form_factor_monitor,
        mir_form_factor_projector,
        mir_form_factor_unknown
    }};

    std::shared_ptr<mg::DisplayConfiguration> current_config = server.the_display()->configuration();
    current_config->for_each_output(
        [&form_factors, output_num = 0](mg::UserDisplayConfigurationOutput& output) mutable
            {
                output.form_factor = form_factors[output_num];
                ++output_num;
            });

    DisplayClient client{new_connection()};

    client.connect();

    mt::Signal configuration_received;
    mir_connection_set_display_config_change_callback(
        client.connection,
        &signal_when_config_received,
        &configuration_received);

    server.the_display_configuration_controller()->set_base_configuration(current_config);

    EXPECT_TRUE(configuration_received.wait_for(std::chrono::seconds{10}));

    auto client_config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_output(client_config.get(), i);

        EXPECT_THAT(mir_output_get_form_factor(output), Eq(form_factors[i]));
    }

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_sees_server_set_gamma)
{
    uint32_t const size = 4;
    mg::GammaCurve const a{0, 1, 2, 3};
    mg::GammaCurve const b{1, 2, 3, 4};
    mg::GammaCurve const c{65532, 65533, 65534, 65535};
    std::vector<mg::GammaCurves> const gammas = {
        {a, b, c},
        {b, c, a},
        {c, a, b}
    };

    std::shared_ptr<mg::DisplayConfiguration> current_config = server.the_display()->configuration();
    current_config->for_each_output(
        [&gammas, output_num = 0](mg::UserDisplayConfigurationOutput& output) mutable
            {
                output.gamma = gammas[output_num];
                ++output_num;
            });

    DisplayClient client{new_connection()};

    client.connect();

    mt::Signal configuration_received;
    mir_connection_set_display_config_change_callback(
        client.connection,
        &signal_when_config_received,
        &configuration_received);

    server.the_display_configuration_controller()->set_base_configuration(current_config);

    EXPECT_TRUE(configuration_received.wait_for(std::chrono::seconds{10}));

    auto client_config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_output(client_config.get(), i);

        ASSERT_THAT(mir_output_get_gamma_size(output), size);

        uint16_t red[size];
        uint16_t green[size];
        uint16_t blue[size];

        mir_output_get_gamma(output, red, green, blue, size);

        for (size_t r = 0; r < size; r++)
        {
            EXPECT_THAT(gammas[i].red[r], red[r]);
        }

        for (size_t g = 0; g < size; g++)
        {
            EXPECT_THAT(gammas[i].green[g], green[g]);
        }

        for (size_t b = 0; b < size; b++)
        {
            EXPECT_THAT(gammas[i].blue[b], blue[b]);
        }
    }

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_can_set_gamma)
{
    mg::GammaCurve const a{0, 1, 2, 3};
    mg::GammaCurve const b{1, 2, 3, 4};
    mg::GammaCurve const c{65532, 65533, 65534, 65535};
    std::vector<mg::GammaCurves> const gammas = {
        {a, b, c},
        {b, c, a},
        {c, a, b}
    };

    DisplayClient client{new_connection()};

    client.connect();

    auto client_config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_mutable_output(client_config.get(), i);

        mir_output_set_gamma(output,
                             gammas[i].red.data(),
                             gammas[i].green.data(),
                             gammas[i].blue.data(),
                             gammas[i].red.size());
    }

    DisplayConfigMatchingContext context;
    context.matcher = [c = client_config.get()](MirDisplayConfig* conf)
        {
            EXPECT_THAT(conf, mt::DisplayConfigMatches(c));
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    mir_connection_preview_base_display_configuration(client.connection, client_config.get(), 10);

    EXPECT_TRUE(context.done.wait_for(std::chrono::seconds{5}));

    mir_connection_confirm_base_display_configuration(client.connection, client_config.get());

    std::shared_ptr<mg::DisplayConfiguration> current_config = server.the_display()->configuration();
    current_config->for_each_output(
        [&gammas, output_num = 0](mg::UserDisplayConfigurationOutput& output) mutable
            {
                for (size_t r = 0; r < output.gamma.red.size(); r++)
                {
                    EXPECT_THAT(output.gamma.red[r], gammas[output_num].red[r]);
                }

                for (size_t g = 0; g < output.gamma.green.size(); g++)
                {
                    EXPECT_THAT(output.gamma.green[g], gammas[output_num].green[g]);
                }

                for (size_t b = 0; b < output.gamma.green.size(); b++)
                {
                    EXPECT_THAT(output.gamma.blue[b], gammas[output_num].blue[b]);
                }

                ++output_num;
            });

    client.disconnect();
}

namespace
{
MATCHER_P(PointsToIdenticalData, data, "")
{
    return arg == nullptr ? false : memcmp(arg, data.data(), data.size()) == 0;
}
}

TEST_F(DisplayConfigurationTest, client_receives_null_for_empty_edid)
{
    mtd::StubDisplayConfigurationOutput monitor{
        mg::DisplayConfigurationOutputId{2},
        {{{3210, 2800}, 60.0}},
        {mir_pixel_format_abgr_8888}};
    monitor.edid = {};

    auto config = std::make_shared<mtd::StubDisplayConfig>(std::vector<mg::DisplayConfigurationOutput>{monitor});

    apply_config_change_and_wait_for_propagation(config);

    DisplayClient client{new_connection()};
    client.connect();

    auto configuration = client.get_base_config();

    auto output = mir_display_config_get_output(configuration.get(), 0);

    EXPECT_THAT(mir_output_get_edid(output), IsNull());

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_receives_edid)
{
    /*
     * Valid EDID block in case we later do some parsing/quirking/validation.
     */
    std::vector<uint8_t> edid{
        0xff, 0x00, 0xff, 0xff, 0xff, 0xff, 0x00, 0xff,
        0xaf, 0x06, 0x11, 0x3d, 0x00, 0x00, 0x00, 0x00,
        0x16, 0x00, 0x04, 0x01, 0x1f, 0x95, 0x78, 0x11,
        0x87, 0x02, 0xa4, 0xe5, 0x50, 0x56, 0x26, 0x9e,
        0x50, 0x0d, 0x00, 0x54, 0x00, 0x00, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
        0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x37, 0x14,
        0xb8, 0x80, 0x38, 0x70, 0x40, 0x24, 0x10, 0x10,
        0x00, 0x3e, 0xad, 0x35, 0x00, 0x10, 0x18, 0x00,
        0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x20, 0x00, 0x00, 0x00, 0xfe, 0x00, 0x41, 0x00,
        0x4f, 0x55, 0x20, 0x0a, 0x20, 0x20, 0x20, 0x20,
        0x20, 0x20, 0x20, 0x20, 0x00, 0x00, 0xfe, 0x00,
        0x42, 0x00, 0x34, 0x31, 0x48, 0x30, 0x4e, 0x41,
        0x31, 0x30, 0x31, 0x2e, 0x0a, 0x20, 0xa5, 0x00
    };

    mtd::StubDisplayConfigurationOutput monitor{
        mg::DisplayConfigurationOutputId{2},
        {{{3210, 2800}, 60.0}},
        {mir_pixel_format_abgr_8888}};
    monitor.edid = edid;

    auto config = std::make_shared<mtd::StubDisplayConfig>(std::vector<mg::DisplayConfigurationOutput>{monitor});

    apply_config_change_and_wait_for_propagation(config);

    DisplayClient client{new_connection()};
    client.connect();

    auto configuration = client.get_base_config();

    auto output = mir_display_config_get_output(configuration.get(), 0);

    EXPECT_THAT(mir_output_get_edid(output), PointsToIdenticalData(edid));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_receives_model_string_from_edid)
{
    static unsigned char const edid[] =
        "\x00\xff\xff\xff\xff\xff\xff\x00\x10\xac\x46\xf0\x4c\x4a\x31\x41"
        "\x05\x19\x01\x04\xb5\x34\x20\x78\x3a\x1d\xf5\xae\x4f\x35\xb3\x25"
        "\x0d\x50\x54\xa5\x4b\x00\x81\x80\xa9\x40\xd1\x00\x71\x4f\x01\x01"
        "\x01\x01\x01\x01\x01\x01\x28\x3c\x80\xa0\x70\xb0\x23\x40\x30\x20"
        "\x36\x00\x06\x44\x21\x00\x00\x1a\x00\x00\x00\xff\x00\x59\x43\x4d"
        "\x30\x46\x35\x31\x52\x41\x31\x4a\x4c\x0a\x00\x00\x00\xfc\x00\x44"
        "\x45\x4c\x4c\x20\x55\x32\x34\x31\x33\x0a\x20\x20\x00\x00\x00\xfd"
        "\x00\x38\x4c\x1e\x51\x11\x00\x0a\x20\x20\x20\x20\x20\x20\x01\x42"
        "\x02\x03\x1d\xf1\x50\x90\x05\x04\x03\x02\x07\x16\x01\x1f\x12\x13"
        "\x14\x20\x15\x11\x06\x23\x09\x1f\x07\x83\x01\x00\x00\x02\x3a\x80"
        "\x18\x71\x38\x2d\x40\x58\x2c\x45\x00\x06\x44\x21\x00\x00\x1e\x01"
        "\x1d\x80\x18\x71\x1c\x16\x20\x58\x2c\x25\x00\x06\x44\x21\x00\x00"
        "\x9e\x01\x1d\x00\x72\x51\xd0\x1e\x20\x6e\x28\x55\x00\x06\x44\x21"
        "\x00\x00\x1e\x8c\x0a\xd0\x8a\x20\xe0\x2d\x10\x10\x3e\x96\x00\x06"
        "\x44\x21\x00\x00\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x09";

    mtd::StubDisplayConfigurationOutput monitor{
        mg::DisplayConfigurationOutputId{48},
        {{{1920, 1200}, 60.0}},
        {mir_pixel_format_abgr_8888}};
    monitor.edid.assign(edid, edid+sizeof(edid)-1);

    auto config = std::make_shared<mtd::StubDisplayConfig>(
                      std::vector<mg::DisplayConfigurationOutput>{monitor});

    apply_config_change_and_wait_for_propagation(config);

    DisplayClient client{new_connection()};
    client.connect();

    auto base_config = client.get_base_config();
    auto output = mir_display_config_get_output(base_config.get(), 0);

    EXPECT_STREQ("DELL U2413", mir_output_get_model(output));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_receives_fallback_string_from_edid)
{
    static unsigned char const edid[] =
        "\x00\xff\xff\xff\xff\xff\xff\x00\x10\xac\x46\xf0\x4c\x4a\x31\x41"
        "\x05\x19\x01\x04\xb5\x34\x20\x78\x3a\x1d\xf5\xae\x4f\x35\xb3\x25"
        "\x0d\x50\x54\xa5\x4b\x00\x81\x80\xa9\x40\xd1\x00\x71\x4f\x01\x01"
        "\x01\x01\x01\x01\x01\x01\x28\x3c\x80\xa0\x70\xb0\x23\x40\x30\x20"
        "\x36\x00\x06\x44\x21\x00\x00\x1a\x00\x00\x00\xff\x00\x59\x43\x4d"
        "\x30\x46\x35\x31\x52\x41\x31\x4a\x4c\x0a\x00\x00\x00\x11\x00\x44"
        "\x45\x4c\x4c\x20\x55\x32\x34\x31\x33\x0a\x20\x20\x00\x00\x00\xfd"
        "\x00\x38\x4c\x1e\x51\x11\x00\x0a\x20\x20\x20\x20\x20\x20\x01\x42"
        "\x02\x03\x1d\xf1\x50\x90\x05\x04\x03\x02\x07\x16\x01\x1f\x12\x13"
        "\x14\x20\x15\x11\x06\x23\x09\x1f\x07\x83\x01\x00\x00\x02\x3a\x80"
        "\x18\x71\x38\x2d\x40\x58\x2c\x45\x00\x06\x44\x21\x00\x00\x1e\x01"
        "\x1d\x80\x18\x71\x1c\x16\x20\x58\x2c\x25\x00\x06\x44\x21\x00\x00"
        "\x9e\x01\x1d\x00\x72\x51\xd0\x1e\x20\x6e\x28\x55\x00\x06\x44\x21"
        "\x00\x00\x1e\x8c\x0a\xd0\x8a\x20\xe0\x2d\x10\x10\x3e\x96\x00\x06"
        "\x44\x21\x00\x00\x18\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
        "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x09";

    mtd::StubDisplayConfigurationOutput monitor{
        mg::DisplayConfigurationOutputId{2},
        {{{3210, 2800}, 60.0}},
        {mir_pixel_format_abgr_8888}};
    monitor.edid.assign(edid, edid+sizeof(edid)-1);

    auto config = std::make_shared<mtd::StubDisplayConfig>(
                      std::vector<mg::DisplayConfigurationOutput>{monitor});

    apply_config_change_and_wait_for_propagation(config);

    DisplayClient client{new_connection()};
    client.connect();

    auto base_config = client.get_base_config();
    auto output = mir_display_config_get_output(base_config.get(), 0);

    EXPECT_STREQ("DEL 61510", mir_output_get_model(output));

    client.disconnect();
}

namespace
{
MATCHER_P(IsSameModeAs, mode, "")
{
    return mir_output_mode_get_height(arg) == mir_output_mode_get_height(mode) &&
        mir_output_mode_get_width(arg) == mir_output_mode_get_width(mode) &&
        mir_output_mode_get_refresh_rate(arg) == mir_output_mode_get_refresh_rate(mode);
}
}

TEST_F(DisplayConfigurationTest, get_current_mode_index_invariants)
{
    DisplayClient client{new_connection()};

    client.connect();

    auto client_config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_output(client_config.get(), i);

        if (auto mode = mir_output_get_current_mode(output))
        {
            auto indexed_mode = mir_output_get_mode(output, mir_output_get_current_mode_index(output));
            EXPECT_THAT(indexed_mode, IsSameModeAs(mode));
        }
        else
        {
            EXPECT_THAT(mir_output_get_current_mode_index(output), Eq(std::numeric_limits<size_t>::max()));
        }
    }

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, get_preferred_mode_index_invariants)
{
    DisplayClient client{new_connection()};

    client.connect();

    auto client_config = client.get_base_config();

    for (int i = 0; i < mir_display_config_get_num_outputs(client_config.get()); ++i)
    {
        auto output = mir_display_config_get_output(client_config.get(), i);

        if (auto mode = mir_output_get_preferred_mode(output))
        {
            auto indexed_mode = mir_output_get_mode(output, mir_output_get_preferred_mode_index(output));
            EXPECT_THAT(indexed_mode, IsSameModeAs(mode));
        }
        else
        {
            EXPECT_THAT(mir_output_get_preferred_mode_index(output), Eq(std::numeric_limits<size_t>::max()));
        }
    }

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, preview_base_display_configuration_sends_config_event)
{
    DisplayClient client{new_connection()};

    client.connect();

    std::shared_ptr<MirDisplayConfig> config = client.get_base_config();

    for (auto i = 0; i < mir_display_config_get_num_outputs(config.get()); ++i)
    {
        auto output = mir_display_config_get_mutable_output(config.get(), i);

        for (auto j = 0; j < mir_output_get_num_modes(output); ++j)
        {
            auto mode = mir_output_get_mode(output, j);

            if (mode != mir_output_get_current_mode(output))
            {
                mir_output_set_current_mode(output, mode);
                break;
            }
        }
    }

    ASSERT_THAT(config.get(), Not(mt::DisplayConfigMatches(client.get_base_config().get())));

    DisplayConfigMatchingContext context;
    context.matcher = [config](MirDisplayConfig* conf)
        {
            EXPECT_THAT(conf, mt::DisplayConfigMatches(config.get()));
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    mir_connection_preview_base_display_configuration(client.connection, config.get(), 5);

    EXPECT_TRUE(context.done.wait_for(std::chrono::seconds{10}));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, preview_base_display_configuration_reverts_after_timeout)
{
    DisplayClient client{new_connection()};

    client.connect();

    std::shared_ptr<MirDisplayConfig> old_config = client.get_base_config();
    std::shared_ptr<MirDisplayConfig> new_config = client.get_base_config();

    for (auto i = 0; i < mir_display_config_get_num_outputs(new_config.get()); ++i)
    {
        auto output = mir_display_config_get_mutable_output(new_config.get(), i);

        for (auto j = 0; j < mir_output_get_num_modes(output); ++j)
        {
            auto mode = mir_output_get_mode(output, j);

            if (mode != mir_output_get_current_mode(output))
            {
                mir_output_set_current_mode(output, mode);
                break;
            }
        }
    }

    ASSERT_THAT(new_config.get(), Not(mt::DisplayConfigMatches(old_config.get())));

    DisplayConfigMatchingContext context;
    auto reverted = std::make_shared<mt::Signal>();
    context.matcher = [old_config, new_config, reverted, call_count = 0](MirDisplayConfig* conf) mutable
        {
            ++call_count;
            if (call_count == 1)
            {
                EXPECT_THAT(conf, mt::DisplayConfigMatches(new_config.get()));
            }
            else if (call_count == 2)
            {
                EXPECT_THAT(conf, mt::DisplayConfigMatches(old_config.get()));
                reverted->raise();
            }
            else
            {
                FAIL() << "Received unexpected configuration event";
            }
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    mir_connection_preview_base_display_configuration(client.connection, new_config.get(), 5);

    std::this_thread::sleep_for(std::chrono::seconds{3});
    // Should still have the old config
    EXPECT_TRUE(context.done.raised());
    EXPECT_FALSE(reverted->raised());

    EXPECT_TRUE(reverted->wait_for(std::chrono::seconds{10}));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, display_configuration_sticks_after_confirmation)
{
    DisplayClient client{new_connection()};

    client.connect();

    std::shared_ptr<MirDisplayConfig> old_config = client.get_base_config();
    std::shared_ptr<MirDisplayConfig> new_config = client.get_base_config();

    for (auto i = 0; i < mir_display_config_get_num_outputs(new_config.get()); ++i)
    {
        auto output = mir_display_config_get_mutable_output(new_config.get(), i);

        for (auto j = 0; j < mir_output_get_num_modes(output); ++j)
        {
            auto mode = mir_output_get_mode(output, j);

            if (mode != mir_output_get_current_mode(output))
            {
                mir_output_set_current_mode(output, mode);
                break;
            }
        }
    }

    ASSERT_THAT(new_config.get(), Not(mt::DisplayConfigMatches(old_config.get())));

    DisplayConfigMatchingContext context;
    auto signalled_twice = std::make_shared<mt::Signal>();
    context.matcher = [new_config, signalled_twice, call_count = 0](MirDisplayConfig* conf) mutable
        {
            ++call_count;
            EXPECT_THAT(conf, mt::DisplayConfigMatches(new_config.get()));
            if (call_count == 2)
            {
                signalled_twice->raise();
            }
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        &new_display_config_matches,
        &context);

    mir_connection_preview_base_display_configuration(client.connection, new_config.get(), 10);

    EXPECT_TRUE(context.done.wait_for(std::chrono::seconds{5}));

    mir_connection_confirm_base_display_configuration(client.connection, new_config.get());

    EXPECT_TRUE(signalled_twice->wait_for(std::chrono::seconds{10}));

    client.disconnect();
}

namespace
{
struct ErrorValidator
{
    mt::Signal received;
    std::function<void(MirError const*)> validate;
};

void validating_error_handler(MirConnection*, MirError const* error, void* context)
{
    auto& error_validator = *reinterpret_cast<ErrorValidator*>(context);
    error_validator.validate(error);
    error_validator.received.raise();
}
}

TEST_F(DisplayConfigurationTest, unauthorised_client_receives_error)
{
    stub_authorizer.allow_set_base_display_configuration = false;

    DisplayClient client{new_connection()};

    client.connect();

    auto config = client.get_base_config();

    ErrorValidator validator;
    validator.validate = [](MirError const* error)
        {
            EXPECT_THAT(mir_error_get_domain(error), Eq(mir_error_domain_display_configuration));
            EXPECT_THAT(mir_error_get_code(error), Eq(mir_display_configuration_error_unauthorized));
        };
    mir_connection_set_error_callback(client.connection, &validating_error_handler, &validator);

    mir_connection_preview_base_display_configuration(client.connection, config.get(), 20);

    EXPECT_TRUE(validator.received.wait_for(std::chrono::seconds{10}));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, can_confirm_base_configuration_without_waiting_for_change_event)
{
    using namespace testing;

    DisplayClient client{new_connection()};

    client.connect();

    auto new_config = client.get_base_config();

    mir_output_set_position(mir_display_config_get_mutable_output(new_config.get(), 0), 500, 12000);

    auto received_new_configuration = std::make_shared<mt::Signal>();
    std::function<void(MirDisplayConfig const*)> validator =
        [received_new_configuration, expected_config = new_config.get()](MirDisplayConfig const* config)
        {
            EXPECT_THAT(config, mt::DisplayConfigMatches(expected_config));
            received_new_configuration->raise();
        };

    mir_connection_set_display_config_change_callback(
        client.connection,
        [](MirConnection* conn, void* ctx)
        {
            auto validator = *reinterpret_cast<std::function<void(MirDisplayConfig const*)>*>(ctx);

            auto config = mir_connection_create_display_configuration(conn);

            validator(config);

            mir_display_config_release(config);
        },
        &validator);

    mir_connection_preview_base_display_configuration(client.connection, new_config.get(), 1);
    mir_connection_confirm_base_display_configuration(client.connection, new_config.get());

    EXPECT_TRUE(received_new_configuration->wait_for(10s));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, receives_error_when_display_configuration_already_in_progress)
{
    DisplayClient client{new_connection()};

    client.connect();

    auto config = client.get_base_config();

    ErrorValidator validator;
    validator.validate = [](MirError const* error)
    {
        EXPECT_THAT(mir_error_get_domain(error), Eq(mir_error_domain_display_configuration));
        EXPECT_THAT(mir_error_get_code(error), Eq(mir_display_configuration_error_in_progress));
    };
    mir_connection_set_error_callback(client.connection, &validating_error_handler, &validator);

    mir_connection_preview_base_display_configuration(client.connection, config.get(), 20);
    mir_connection_preview_base_display_configuration(client.connection, config.get(), 20);

    EXPECT_TRUE(validator.received.wait_for(std::chrono::seconds{10}));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, can_cancel_base_display_configuration_preview)
{
    using namespace std::chrono_literals;

    DisplayClient client{new_connection()};

    client.connect();

    auto old_config = client.get_base_config();
    auto new_config = client.get_base_config();

    mir_output_set_position(mir_display_config_get_mutable_output(new_config.get(), 0), 88, 42);

    struct ConfigurationExpectation
    {
        std::atomic<MirDisplayConfig const*> configuration;
        mt::Signal satisfied;
    } expectation;

    mir_connection_set_display_config_change_callback(
        client.connection,
        [](MirConnection* connection, void* ctx)
        {
            auto expectation = reinterpret_cast<ConfigurationExpectation*>(ctx);

            auto config = mir_connection_create_display_configuration(connection);
            if (Matches(mt::DisplayConfigMatches(expectation->configuration.load()))(config))
            {
                expectation->satisfied.raise();
            }
            mir_display_config_release(config);
        },
        &expectation);

    expectation.configuration = new_config.get();
    mir_connection_preview_base_display_configuration(client.connection, new_config.get(), 20);

    EXPECT_TRUE(expectation.satisfied.wait_for(10s));
    expectation.satisfied.reset();

    expectation.configuration = old_config.get();
    mir_connection_cancel_base_display_configuration_preview(client.connection);

    EXPECT_TRUE(expectation.satisfied.wait_for(10s));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, cancel_receives_error_when_no_preview_pending)
{
    DisplayClient client{new_connection()};

    client.connect();

    auto config = client.get_base_config();

    ErrorValidator validator;
    validator.validate = [](MirError const* error)
    {
        EXPECT_THAT(mir_error_get_domain(error), Eq(mir_error_domain_display_configuration));
        EXPECT_THAT(mir_error_get_code(error), Eq(mir_display_configuration_error_no_preview_in_progress));
    };
    mir_connection_set_error_callback(client.connection, &validating_error_handler, &validator);

    mir_connection_cancel_base_display_configuration_preview(client.connection);

    EXPECT_TRUE(validator.received.wait_for(std::chrono::seconds{10}));

    validator.received.reset();

    mir_connection_preview_base_display_configuration(client.connection, config.get(), 20);
    mir_connection_confirm_base_display_configuration(client.connection, config.get());

    mir_connection_cancel_base_display_configuration_preview(client.connection);

    EXPECT_TRUE(validator.received.wait_for(std::chrono::seconds{10}));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, client_receives_exactly_one_configuration_event_after_cancel)
{
    using namespace std::chrono_literals;

    DisplayClient client{new_connection()};

    client.connect();

    auto old_config = client.get_base_config();
    auto new_config = client.get_base_config();

    mir_output_set_position(mir_display_config_get_mutable_output(new_config.get(), 0), 88, 42);

    struct ConfigurationExpectation
    {
        std::atomic<MirDisplayConfig const*> configuration;
        mt::Signal satisfied;
    } expectation;

    mir_connection_set_display_config_change_callback(
        client.connection,
        [](MirConnection* connection, void* ctx)
        {
            auto expectation = reinterpret_cast<ConfigurationExpectation*>(ctx);

            auto config = mir_connection_create_display_configuration(connection);
            if (Matches(mt::DisplayConfigMatches(expectation->configuration.load()))(config))
            {
                if (expectation->satisfied.raised())
                {
                    FAIL() << "Received more than one display configuration event matching filter";
                }
                expectation->satisfied.raise();
            }
            mir_display_config_release(config);
        },
        &expectation);

    expectation.configuration = new_config.get();

    auto timeout_time = std::chrono::steady_clock::now() + 2s;
    mir_connection_preview_base_display_configuration(client.connection, new_config.get(), 2);

    EXPECT_TRUE(expectation.satisfied.wait_for(2s));
    expectation.satisfied.reset();

    expectation.configuration = old_config.get();
    mir_connection_cancel_base_display_configuration_preview(client.connection);

    EXPECT_TRUE(expectation.satisfied.wait_for(10s));

    // Sleep until a bit after the preview would timeout to check that the timeout doesn't
    // send a second event.
    std::this_thread::sleep_until(timeout_time + 1s);

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, cancel_doesnt_affect_other_clients_configuration_in_progress)
{
    using namespace std::chrono_literals;

    DisplayClient configuring_client{new_connection()};
    DisplayClient interfering_client{new_connection()};

    configuring_client.connect();
    interfering_client.connect();

    auto old_config = configuring_client.get_base_config();
    auto new_config = configuring_client.get_base_config();

    mir_output_set_position(mir_display_config_get_mutable_output(new_config.get(), 0), 88, 42);

    struct ConfigurationExpectation
    {
        std::atomic<MirDisplayConfig const*> configuration;
        mt::Signal satisfied;
    } expectation;

    mir_connection_set_display_config_change_callback(
        configuring_client.connection,
        [](MirConnection* connection, void* ctx)
        {
            auto expectation = reinterpret_cast<ConfigurationExpectation*>(ctx);

            auto config = mir_connection_create_display_configuration(connection);
            if (Matches(mt::DisplayConfigMatches(expectation->configuration.load()))(config))
            {
                expectation->satisfied.raise();
            }
            mir_display_config_release(config);
        },
        &expectation);

    expectation.configuration = new_config.get();
    mir_connection_preview_base_display_configuration(configuring_client.connection, new_config.get(), 20);

    EXPECT_TRUE(expectation.satisfied.wait_for(10s));
    expectation.satisfied.reset();

    mt::Signal error_received;
    mir_connection_set_error_callback(
        interfering_client.connection,
        [](MirConnection*, MirError const* error, void* ctx)
        {
            auto const done = reinterpret_cast<mt::Signal*>(ctx);

            if (mir_error_get_domain(error) == mir_error_domain_display_configuration)
            {
                EXPECT_THAT(mir_error_get_code(error), Eq(mir_display_configuration_error_no_preview_in_progress));
                done->raise();
            }
        },
        &error_received);

    mir_connection_cancel_base_display_configuration_preview(interfering_client.connection);

    EXPECT_TRUE(error_received.wait_for(10s));
    EXPECT_THAT(configuring_client.get_base_config().get(), mt::DisplayConfigMatches(new_config.get()));

    expectation.configuration = old_config.get();
    mir_connection_cancel_base_display_configuration_preview(configuring_client.connection);

    EXPECT_TRUE(expectation.satisfied.wait_for(10s));

    configuring_client.disconnect();
    interfering_client.disconnect();
}

TEST_F(DisplayConfigurationTest, error_in_configure_when_previewing_propagates_to_client)
{
    using namespace std::chrono_literals;
    DisplayClient client{new_connection()};
    client.connect();

    ON_CALL(mock_display, configure(_))
        .WillByDefault(
            InvokeWithoutArgs(
                []() { BOOST_THROW_EXCEPTION(std::runtime_error("Ducks!")); } ));

    mt::Signal error_received;
    mir_connection_set_error_callback(
        client.connection,
        [](MirConnection*, MirError const* error, void* ctx)
        {
            if (mir_error_get_domain(error) == mir_error_domain_display_configuration)
            {
                EXPECT_THAT(mir_error_get_code(error), Eq(mir_display_configuration_error_rejected_by_hardware));
                reinterpret_cast<mt::Signal*>(ctx)->raise();
            }
        },
        &error_received);

    auto config = client.get_base_config();
    mir_output_set_position(mir_display_config_get_mutable_output(config.get(), 0), 88, 42);

    mir_connection_preview_base_display_configuration(client.connection, config.get(), 100);

    EXPECT_TRUE(error_received.wait_for(10s));

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, configure_session_display)
{
    auto configuration = mir_connection_create_display_configuration(connection);

    EXPECT_CALL(*observer, session_configuration_applied(_, _))
        .Times(1)
        .WillOnce(mt::WakeUp(&observed_changed));

    mir_connection_apply_session_display_config(connection, configuration);

    observed_changed.wait_for(10s);

    mir_display_config_release(configuration);
}

TEST_F(DisplayConfigurationTest, configure_session_removed_display)
{
    auto configuration = mir_connection_create_display_configuration(connection);

    EXPECT_CALL(*observer, session_configuration_applied(_, _))
        .Times(1)
        .WillOnce(mt::WakeUp(&observed_changed));

    mir_connection_apply_session_display_config(connection, configuration);

    observed_changed.wait_for(10s);
    observed_changed.reset();

    EXPECT_CALL(*observer, session_configuration_removed(_))
        .Times(1)
        .WillOnce(mt::WakeUp(&observed_changed));

    mir_connection_remove_session_display_config(connection);

    observed_changed.wait_for(10s);

    mir_display_config_release(configuration);
}

TEST_F(DisplayConfigurationTest, remove_is_noop_when_no_session_configuration_set)
{
    EXPECT_CALL(*observer, session_configuration_removed(_))
        .Times(0);

    mir_connection_remove_session_display_config(connection);

    std::this_thread::sleep_for(1s);
}

TEST_F(DisplayConfigurationTest, remove_from_focused_client_causes_hardware_change)
{
    DisplayClient client{new_connection()};

    client.connect();

    std::atomic<int> times{2};
    auto new_config = mir_connection_create_display_configuration(client.connection);

    // Apply a configuration to the client, assert it has been configured for the display
    {
        mir_output_set_position(mir_display_config_get_mutable_output(new_config, 0), 500, 12000);

        EXPECT_CALL(*observer, session_configuration_applied(_, mt::DisplayConfigMatches(new_config)))
            .Times(1)
            .WillOnce(mt::WakeUpWhenZero(&observed_changed, &times));
        EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(new_config)))
            .Times(1)
            .WillOnce(mt::WakeUpWhenZero(&observed_changed, &times));

        mir_connection_apply_session_display_config(client.connection, new_config);

        observed_changed.wait_for(10s);
        observed_changed.reset();
        mir_display_config_release(new_config);
    }

    // Remove the configuration, assert we have been re-configured back to the base config
    {
        times = 2;

        EXPECT_CALL(*observer, session_configuration_removed(_))
            .Times(1)
            .WillOnce(mt::WakeUpWhenZero(&observed_changed, &times));
        EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(std::cref(stub_display_config))))
            .Times(1)
            .WillOnce(mt::WakeUpWhenZero(&observed_changed, &times));

        mir_connection_remove_session_display_config(client.connection);

        observed_changed.wait_for(10s);
    }

    client.disconnect();
}

TEST_F(DisplayConfigurationTest, remove_from_unfocused_client_causes_no_hardware_change)
{
    DisplayClient client{new_connection()};

    client.connect();

    auto new_config = mir_connection_create_display_configuration(client.connection);
    std::atomic<int> times{2};

    // Set the first clients display config
    {
        mir_output_set_position(mir_display_config_get_mutable_output(new_config, 0), 500, 12000);

        EXPECT_CALL(*observer, session_configuration_applied(_, mt::DisplayConfigMatches(new_config)))
            .Times(1)
            .WillOnce(mt::WakeUpWhenZero(&observed_changed, &times));
        EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(new_config)))
            .Times(1)
            .WillOnce(mt::WakeUpWhenZero(&observed_changed, &times));

        mir_connection_apply_session_display_config(client.connection, new_config);

        observed_changed.wait_for(10s);
        observed_changed.reset();
        mir_display_config_release(new_config);
    }

    DisplayClient client2{new_connection()};

    // Connect and wait for the second client to get setup and config. Which will be the focused session
    {
        EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(std::cref(stub_display_config))))
            .Times(1)
            .WillOnce(mt::WakeUp(&observed_changed));

        client2.connect();
        observed_changed.wait_for(10s);
        observed_changed.reset();
    }

    // Remove the display config from the first config and assert no hardware changes happen
    {
        EXPECT_CALL(*observer, session_configuration_removed(_))
            .Times(1)
            .WillOnce(mt::WakeUp(&observed_changed));
        EXPECT_CALL(mock_display, configure(_))
            .Times(0);

        mir_connection_remove_session_display_config(client.connection);

        observed_changed.wait_for(10s);
        std::this_thread::sleep_for(1s);
    }

    client2.disconnect();
    client.disconnect();
}

TEST_F(DisplayConfigurationTest, remove_from_unfocused_client_causes_hardware_change_when_focused)
{
    DisplayClient client{new_connection()};

    client.connect();

    auto new_config = mir_connection_create_display_configuration(client.connection);
    std::atomic<int> times{2};

    // Set the first clients display config
    {
        mir_output_set_position(mir_display_config_get_mutable_output(new_config, 0), 500, 12000);

        EXPECT_CALL(*observer, session_configuration_applied(_, mt::DisplayConfigMatches(new_config)))
            .Times(1)
            .WillOnce(mt::WakeUpWhenZero(&observed_changed, &times));
        EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(new_config)))
            .Times(1)
            .WillOnce(mt::WakeUpWhenZero(&observed_changed, &times));

        mir_connection_apply_session_display_config(client.connection, new_config);

        observed_changed.wait_for(10s);
        observed_changed.reset();
    }

    DisplayClient client2{new_connection()};

    // Connect and wait for the second client to get setup and config. Which will be the focused session
    {
        EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(std::cref(stub_display_config))))
            .Times(1)
            .WillOnce(mt::WakeUp(&observed_changed));

        client2.connect();
        observed_changed.wait_for(10s);
        observed_changed.reset();
    }

    // Apply the new_config to client 2 as well so we can see disconnecting will apply the base config
    {
        times = 2;

        EXPECT_CALL(*observer, session_configuration_applied(_, mt::DisplayConfigMatches(new_config)))
            .Times(1)
            .WillOnce(mt::WakeUpWhenZero(&observed_changed, &times));
        EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(new_config)))
            .Times(1)
            .WillOnce(mt::WakeUpWhenZero(&observed_changed, &times));

        mir_connection_apply_session_display_config(client2.connection, new_config);
        observed_changed.wait_for(10s);
        observed_changed.reset();
        mir_display_config_release(new_config);
    }

    // Remove the display config from the first config and assert no hardware changes happen
    {

        EXPECT_CALL(*observer, session_configuration_removed(_))
            .Times(1)
            .WillOnce(mt::WakeUp(&observed_changed));
        EXPECT_CALL(mock_display, configure(_))
            .Times(0);

        mir_connection_remove_session_display_config(client.connection);

        observed_changed.wait_for(10s);
        std::this_thread::sleep_for(1s);
        observed_changed.reset();
    }

    testing::Mock::VerifyAndClearExpectations(&mock_display);

    // Disconnect client 2 which makes client 1 regain focus. Assert the base config is configured
    {
        EXPECT_CALL(mock_display, configure(mt::DisplayConfigMatches(std::cref(stub_display_config))))
            .Times(1)
            .WillOnce(mt::WakeUp(&observed_changed));

        client2.disconnect();

        observed_changed.wait_for(10s);
    }

    client.disconnect();
}
