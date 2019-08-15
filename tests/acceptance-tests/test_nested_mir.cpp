/*
 * Copyright © 2013-2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/session_mediator_observer.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/cursor_image.h"
#include "mir/input/cursor_images.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_configuration_observer.h"
#include "mir/input/input_device_info.h"
#include "mir/shell/shell.h"
#include "mir/input/cursor_listener.h"
#include "mir/cached_ptr.h"
#include "mir/main_loop.h"
#include "mir/server_status_listener.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/shell/display_configuration_controller.h"
#include "mir/shell/host_lifecycle_event_listener.h"
#include "mir/observer_registrar.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/input_device_faker.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/headless_nested_server_runner.h"
#include "mir_test_framework/any_surface.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/observant_shell.h"
#include "mir_test_framework/passthrough_tracker.h"
#include "mir/test/spin_wait.h"
#include "mir/test/display_config_matchers.h"
#include "mir/test/doubles/fake_display.h"
#include "mir/test/doubles/stub_cursor.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/signal_actions.h"

#include "mir/test/doubles/nested_mock_egl.h"
#include "mir/test/fake_shared.h"

#include <linux/input.h>
#include <atomic>
#include <future>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;
namespace msc = mir::scene;

using namespace testing;
using namespace std::chrono_literals;

namespace
{
struct MockSessionMediatorReport : mf::SessionMediatorObserver
{
    MockSessionMediatorReport()
    {
        EXPECT_CALL(*this, session_connect_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_disconnect_called(_)).Times(AnyNumber());

        // These are not needed for the 1st test, but they will be soon
        EXPECT_CALL(*this, session_create_surface_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_release_surface_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_swap_buffers_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_submit_buffer_called(_)).Times(AnyNumber());
    }

    MOCK_METHOD1(session_connect_called, void (std::string const&));
    MOCK_METHOD1(session_create_surface_called, void (std::string const&));
    MOCK_METHOD1(session_swap_buffers_called, void (std::string const&));
    MOCK_METHOD1(session_exchange_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_submit_buffer_called, void (std::string const&));
    MOCK_METHOD1(session_allocate_buffers_called, void (std::string const&));
    MOCK_METHOD1(session_release_buffers_called, void (std::string const&));
    MOCK_METHOD1(session_release_surface_called, void (std::string const&));
    MOCK_METHOD1(session_disconnect_called, void (std::string const&));
    MOCK_METHOD2(session_start_prompt_session_called, void (std::string const&, pid_t));
    MOCK_METHOD1(session_stop_prompt_session_called, void (std::string const&));

    void session_configure_surface_called(std::string const&) override {};
    void session_configure_surface_cursor_called(std::string const&) override {};
    void session_configure_display_called(std::string const&) override {};
    void session_set_base_display_configuration_called(std::string const&) override {};
    void session_preview_base_display_configuration_called(std::string const&) override {};
    void session_confirm_base_display_configuration_called(std::string const&) override {};
    void session_create_buffer_stream_called(std::string const&) override {}
    void session_release_buffer_stream_called(std::string const&) override {}
    void session_error(const std::string&, const char*, const std::string&) override {};
};

struct MockCursor : public mtd::StubCursor
{
MOCK_METHOD1(show, void(mg::CursorImage const&));
};

struct MockHostLifecycleEventListener : msh::HostLifecycleEventListener
{
MOCK_METHOD1(lifecycle_event_occurred, void (MirLifecycleState));
};

struct MockDisplayConfigurationReport : public mg::DisplayConfigurationObserver
{
    MOCK_METHOD1(initial_configuration, void (std::shared_ptr<mg::DisplayConfiguration const> const& configuration));
    MOCK_METHOD1(configuration_applied, void (std::shared_ptr<mg::DisplayConfiguration const> const& configuration));
    MOCK_METHOD2(session_configuration_applied, void (std::shared_ptr<ms::Session> const& session, std::shared_ptr<mg::DisplayConfiguration> const& configuration));
    MOCK_METHOD1(session_configuration_removed, void (std::shared_ptr<ms::Session> const& session));
    MOCK_METHOD1(base_configuration_updated, void (std::shared_ptr<mg::DisplayConfiguration const> const& base_config));
    MOCK_METHOD2(configuration_failed, void(std::shared_ptr<mg::DisplayConfiguration const> const&, std::exception const&));
    MOCK_METHOD2(catastrophic_configuration_error, void(std::shared_ptr<mg::DisplayConfiguration const> const&, std::exception const&));
};

std::vector<geom::Rectangle> const display_geometry
{
    {{  0, 0}, { 640,  480}},
    {{640, 0}, {1920, 1080}}
};

std::vector<geom::Rectangle> const single_display_geometry
{
    {{  0, 0}, { 640,  480}}
};

std::chrono::seconds const timeout{5};
std::chrono::seconds const long_timeout{10};

// We can't rely on the test environment to have X cursors, so we provide some of our own
auto const cursor_names = {
//        mir_disabled_cursor_name,
    mir_arrow_cursor_name,
    mir_busy_cursor_name,
    mir_caret_cursor_name,
    mir_default_cursor_name,
    mir_pointing_hand_cursor_name,
    mir_open_hand_cursor_name,
    mir_closed_hand_cursor_name,
    mir_horizontal_resize_cursor_name,
    mir_vertical_resize_cursor_name,
    mir_diagonal_resize_bottom_to_top_cursor_name,
    mir_diagonal_resize_top_to_bottom_cursor_name,
    mir_omnidirectional_resize_cursor_name,
    mir_vsplit_resize_cursor_name,
    mir_hsplit_resize_cursor_name,
    mir_crosshair_cursor_name };

int const cursor_size = 24;

struct CursorImage : mg::CursorImage
{
    CursorImage(char const* name) :
        id{std::find(begin(cursor_names), end(cursor_names), name) - begin(cursor_names)},
        data{{uint8_t(id)}}
    {
    }

    void const* as_argb_8888() const { return data.data(); }

    geom::Size size() const { return {cursor_size, cursor_size}; }

    geom::Displacement hotspot() const { return {0, 0}; }

    ptrdiff_t id;
    std::array<uint8_t, MIR_BYTES_PER_PIXEL(mir_pixel_format_argb_8888) * cursor_size * cursor_size> data;
};

struct CursorImages : mir::input::CursorImages
{
public:

    std::shared_ptr<mg::CursorImage> image(std::string const& cursor_name, geom::Size const& /*size*/)
    {
        return std::make_shared<CursorImage>(cursor_name.c_str());
    }
};

struct CursorWrapper : mg::Cursor
{
    CursorWrapper(std::shared_ptr<mg::Cursor> const& wrapped) : wrapped{wrapped} {}
    void show() override { if (!hidden) wrapped->show(); }
    void show(mg::CursorImage const& cursor_image) override { if (!hidden) wrapped->show(cursor_image); }
    void hide() override { wrapped->hide(); }

    void move_to(geom::Point position) override { wrapped->move_to(position); }

    void set_hidden(bool hidden)
    {
        this->hidden = hidden;
        if (hidden) hide();
        else show();
    }

private:
    std::shared_ptr<mg::Cursor> const wrapped;
    bool hidden{false};
};

struct MockDisplayConfigurationPolicy : mg::DisplayConfigurationPolicy
{
    MOCK_METHOD1(apply_to, void (mg::DisplayConfiguration&));
};

struct MockDisplay : mtd::FakeDisplay
{
    using mtd::FakeDisplay::FakeDisplay;

    MOCK_METHOD1(configure, void (mg::DisplayConfiguration const&));
};


struct StubSurfaceObserver : msc::NullSurfaceObserver
{
    void cursor_image_set_to(msc::Surface const*, mg::CursorImage const&) override {}
    void cursor_image_removed(msc::Surface const*) override
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        removed = true;
        cv.notify_all();
    }

    bool wait_for_removal()
    {
        using namespace std::chrono_literals;
        std::unique_lock<decltype(mutex)> lk(mutex);
        auto rc = cv.wait_for(lk, 4s, [this] { return removed; });
        return rc;
    }
    std::mutex mutex;
    std::condition_variable cv;
    bool removed = false;
};

class NestedMirRunner : public mtf::HeadlessNestedServerRunner, mtf::InputDeviceFaker
{
public:
    NestedMirRunner(std::string const& connection_string)
        : NestedMirRunner(connection_string, true)
    {
        start_server();

        fake_input_device = add_fake_input_device(mi::InputDeviceInfo{
                "test-devce", "test-device",
                mi::DeviceCapability::pointer | mi::DeviceCapability::keyboard | mi::DeviceCapability::alpha_numeric});

        wait_for_input_devices_added_to(server);
    }

    virtual ~NestedMirRunner()
    {
        stop_server();
    }

    std::shared_ptr<MockHostLifecycleEventListener> the_mock_host_lifecycle_event_listener()
    {
        return mock_host_lifecycle_event_listener([]
            { return std::make_shared<NiceMock<MockHostLifecycleEventListener>>(); });
    }

    std::shared_ptr<CursorWrapper> cursor_wrapper;

    virtual std::shared_ptr<MockDisplayConfigurationPolicy> mock_display_configuration_policy()
    {
        return mock_display_configuration_policy_([]
            { return std::make_shared<NiceMock<MockDisplayConfigurationPolicy>>(); });
    }

    void wait_until_surface_ready(MirWindow* window)
    {
        mir_window_set_event_handler(window, wait_for_key_a_event, this);

        auto const dummy_events_received = mt::spin_wait_for_condition_or_timeout(
            [this]
            {
                if (surface_ready) return true;
                ++probe_scancode;
                fake_input_device->emit_event(
                    mi::synthesis::a_key_down_event().of_scancode(probe_scancode));
                fake_input_device->emit_event(
                    mi::synthesis::a_key_up_event().of_scancode(probe_scancode));
                return false;
            },
            std::chrono::seconds{5},
            std::chrono::seconds{1});

        EXPECT_TRUE(dummy_events_received);

        mir_window_set_event_handler(window, nullptr, nullptr);
    }

protected:
    NestedMirRunner(std::string const& connection_string, bool) :
        mtf::HeadlessNestedServerRunner(connection_string)
    {
        server.override_the_host_lifecycle_event_listener([this]
            { return the_mock_host_lifecycle_event_listener(); });

        server.wrap_cursor([&](std::shared_ptr<mg::Cursor> const& wrapped)
            { return cursor_wrapper = std::make_shared<CursorWrapper>(wrapped); });

        server.override_the_cursor_images([] { return std::make_shared<CursorImages>(); });

        server.wrap_display_configuration_policy([this]
            (std::shared_ptr<mg::DisplayConfigurationPolicy> const&)
            { return mock_display_configuration_policy(); });
    }

private:
    static void wait_for_key_a_event(MirWindow*, MirEvent const* ev, void* context)
    {
        auto const nmr = static_cast<NestedMirRunner*>(context);
        if (mir_event_get_type(ev) == mir_event_type_input)
        {
            auto const iev = mir_event_get_input_event(ev);
            if (mir_input_event_get_type(iev) == mir_input_event_type_key)
            {
                auto const kev = mir_input_event_get_keyboard_event(iev);
                if (mir_keyboard_event_scan_code(kev) == nmr->probe_scancode &&
                    mir_keyboard_event_action(kev) ==mir_keyboard_action_up)
                    nmr->surface_ready = true;
            }
        }
    }

    mir::CachedPtr<MockHostLifecycleEventListener> mock_host_lifecycle_event_listener;
    mir::CachedPtr<MockDisplayConfigurationPolicy> mock_display_configuration_policy_;
    mir::UniqueModulePtr<mtf::FakeInputDevice> fake_input_device;
    std::atomic<bool> surface_ready{false};
    std::atomic<int> probe_scancode{KEY_A};
};

struct NestedServer : mtf::HeadlessInProcessServer
{
    mtd::NestedMockEGL mock_egl;
    mt::Signal condition;
    mt::Signal test_processed_result;

    std::shared_ptr<MockSessionMediatorReport> mock_session_mediator_report
        {std::make_shared<NiceMock<MockSessionMediatorReport>>()};
    std::vector<geom::Rectangle> display_rectangles;
    NiceMock<MockDisplay> display;
    std::shared_ptr<StubSurfaceObserver> stub_observer = std::make_shared<StubSurfaceObserver>();
    NestedServer()
        : display_rectangles{single_display_geometry}, display{display_rectangles}
    {}

    NestedServer(std::vector<geom::Rectangle> const& rectangles)
        : display_rectangles{rectangles}, display{display_rectangles}
    {}

    void SetUp() override
    {
        preset_display(mt::fake_shared(display));

        server.wrap_cursor([this](std::shared_ptr<mg::Cursor> const&) { return the_mock_cursor(); });

        server.wrap_shell([&, this](auto const& wrapped)
        {
            return std::make_shared<mtf::ObservantShell>(wrapped, stub_observer);
        });

        mtf::HeadlessInProcessServer::SetUp();

        server.the_session_mediator_observer_registrar()->register_interest(mock_session_mediator_report);
        server.the_display_configuration_observer_registrar()->register_interest(the_mock_display_configuration_report());
    }

    void trigger_lifecycle_event(MirLifecycleState const lifecycle_state)
    {
        auto const app = server.the_session_coordinator()->successor_of({});

        EXPECT_TRUE(app != nullptr) << "Nested server not connected";

        if (app)
        {
           app->set_lifecycle_state(lifecycle_state);
        }
    }

    std::shared_ptr<MockDisplayConfigurationReport> the_mock_display_configuration_report()
    {
        return mock_display_configuration_report;
    }

    std::shared_ptr<MockDisplayConfigurationReport> const
        mock_display_configuration_report{std::make_shared<NiceMock<MockDisplayConfigurationReport>>()};

    std::shared_ptr<MockCursor> the_mock_cursor()
    {
        return mock_cursor([] { return std::make_shared<NiceMock<MockCursor>>(); });
    }

    mir::CachedPtr<MockCursor> mock_cursor;

    void ignore_rebuild_of_egl_context()
    {
        InSequence context_lifecycle;
        EXPECT_CALL(mock_egl, eglCreateContext(_, _, _, _)).Times(AnyNumber()).WillRepeatedly(Return((EGLContext)this));
        EXPECT_CALL(mock_egl, eglDestroyContext(_, _)).Times(AnyNumber()).WillRepeatedly(Return(EGL_TRUE));
    }

    auto hw_display_config_for_unplug() -> std::shared_ptr<mtd::StubDisplayConfig>
    {
        auto new_displays = display_rectangles;
        new_displays.resize(1);

        return std::make_shared<mtd::StubDisplayConfig>(new_displays);
    }


    auto hw_display_config_for_plugin() -> std::shared_ptr<mtd::StubDisplayConfig>
    {
        auto new_displays = display_rectangles;
        new_displays.push_back({{2560, 0}, { 640,  480}});

        return  std::make_shared<mtd::StubDisplayConfig>(new_displays);
    }

    void change_display_configuration(NestedMirRunner& nested_mir, float scale, MirFormFactor form_factor)
    {
        auto const configurator = nested_mir.server.the_display_configuration_controller();
        std::shared_ptr<mg::DisplayConfiguration> config = nested_mir.server.the_display()->configuration();

        config->for_each_output([scale, form_factor] (mg::UserDisplayConfigurationOutput& output)
        {
            output.scale = scale;
            output.form_factor = form_factor;
        });

        configurator->set_base_configuration(config);
    }

    bool wait_for_display_configuration_change(NestedMirRunner& nested_mir, float expected_scale, MirFormFactor expected_form_factor)
    {
        using namespace std::literals::chrono_literals;
        /* Now, because we have absolutely no idea when the call to set_base_configuration will *actually*
        * set the base configuration we get to poll configuration() until we see that it's actually changed.
        */
       auto const end_time = std::chrono::steady_clock::now() + 10s;
       bool done{false};
       while (!done && (std::chrono::steady_clock::now() < end_time))
       {
           auto const new_config = nested_mir.server.the_display()->configuration();

           new_config->for_each_output([&done, expected_scale, expected_form_factor] (auto const& output)
           {
               if (output.scale == expected_scale &&
                   output.form_factor == expected_form_factor)
               {
                   done = true;
               }
           });
           if (!done)
               std::this_thread::sleep_for(100ms);
       }
       return done;
    }
};

struct NestedServerWithTwoDisplays : NestedServer
{
    NestedServerWithTwoDisplays()
        : NestedServer{display_geometry}
    {
    }
};

struct Client
{
    explicit Client(NestedMirRunner& nested_mir) :
        connection(mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__))
    {}

    ~Client() { mir_connection_release(connection); }

    void update_display_configuration(void (*changer)(MirDisplayConfig* config))
    {
        auto const configuration = mir_connection_create_display_configuration(connection);
        changer(configuration);
        mir_connection_apply_session_display_config(connection, configuration);
        mir_display_config_release(configuration);
    }

    void update_display_configuration_applied_to(MockDisplay& display, void (*changer)(MirDisplayConfig* config))
    {
        mt::Signal initial_condition;

        auto const configuration = mir_connection_create_display_configuration(connection);

        changer(configuration);

        EXPECT_CALL(display, configure(Not(mt::DisplayConfigMatches(configuration)))).Times(AnyNumber())
            .WillRepeatedly(InvokeWithoutArgs([] {}));
        EXPECT_CALL(display, configure(mt::DisplayConfigMatches(configuration)))
            .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.raise(); }));

        mir_connection_apply_session_display_config(connection, configuration);

        initial_condition.wait_for(timeout);
        mir_display_config_release(configuration);

        Mock::VerifyAndClearExpectations(&display);
        ASSERT_TRUE(initial_condition.raised());
    }

    MirConnection* const connection;
};

struct ClientWithADisplayChangeCallback : virtual Client
{
    ClientWithADisplayChangeCallback(NestedMirRunner& nested_mir, MirDisplayConfigCallback callback, void* context) :
        Client(nested_mir)
    {
        mir_connection_set_display_config_change_callback(connection, callback, context);
    }
};

struct ClientWithAPaintedSurface : virtual Client
{
    ClientWithAPaintedSurface(NestedMirRunner& nested_mir, geom::Size size, MirPixelFormat format) :
        Client(nested_mir),
        window(mtf::make_surface(connection, size, format))
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop
    }

    ClientWithAPaintedSurface(NestedMirRunner& nested_mir) :
        Client(nested_mir),
        window(mtf::make_any_surface(connection))
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop
    }

    ~ClientWithAPaintedSurface()
    {
        mir_window_release_sync(window);
    }

    void update_surface_spec(void (*changer)(MirWindowSpec* spec))
    {
        auto const spec = mir_create_window_spec(connection);
        changer(spec);
        mir_window_apply_spec(window, spec);
        mir_window_spec_release(spec);

    }

    MirWindow* window;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
struct ClientWithAPaintedSurfaceAndABufferStream : virtual Client, ClientWithAPaintedSurface
{
    ClientWithAPaintedSurfaceAndABufferStream(NestedMirRunner& nested_mir) :
        Client(nested_mir),
        ClientWithAPaintedSurface(nested_mir),
        buffer_stream(mir_connection_create_buffer_stream_sync(
            connection,
            cursor_size,
            cursor_size,
            mir_pixel_format_argb_8888, mir_buffer_usage_software))
    {
        mir_buffer_stream_swap_buffers_sync(buffer_stream);
    }

    ~ClientWithAPaintedSurfaceAndABufferStream()
    {
        mir_buffer_stream_release_sync(buffer_stream);
    }

    MirBufferStream* const buffer_stream;
};

struct ClientWithAPaintedWindowAndASurface : virtual Client, ClientWithAPaintedSurface
{
    ClientWithAPaintedWindowAndASurface(NestedMirRunner& nested_mir) :
        Client(nested_mir),
        ClientWithAPaintedSurface(nested_mir),
        surface(mir_connection_create_render_surface_sync(
            connection,
            cursor_size, cursor_size)),
        buffer_stream(mir_render_surface_get_buffer_stream(
            surface,
            cursor_size, cursor_size,
            mir_pixel_format_argb_8888))
    {
        mir_buffer_stream_swap_buffers_sync(buffer_stream);
    }

    ~ClientWithAPaintedWindowAndASurface()
    {
        mir_render_surface_release(surface);
    }

    MirRenderSurface* const surface;
    MirBufferStream* const buffer_stream;
};
#pragma GCC diagnostic pop

struct ClientWithADisplayChangeCallbackAndAPaintedSurface : virtual Client, ClientWithADisplayChangeCallback, ClientWithAPaintedSurface
{
    ClientWithADisplayChangeCallbackAndAPaintedSurface(NestedMirRunner& nested_mir, MirDisplayConfigCallback callback, void* context) :
        Client(nested_mir),
        ClientWithADisplayChangeCallback(nested_mir, callback, context),
        ClientWithAPaintedSurface(nested_mir)
    {
    }
};
}

TEST_F(NestedServer, nested_platform_connects_and_disconnects)
{
    mt::Signal signal;
    InSequence seq;
    EXPECT_CALL(*mock_session_mediator_report, session_connect_called(_)).Times(1);
    EXPECT_CALL(*mock_session_mediator_report, session_disconnect_called(_))
        .Times(1)
        .WillOnce(mt::WakeUp(&signal));

    NestedMirRunner{new_connection()};

    EXPECT_TRUE(signal.wait_for(30s));
}

TEST_F(NestedServerWithTwoDisplays, DISABLED_sees_expected_outputs)
{
    NestedMirRunner nested_mir{new_connection()};

    auto const display = nested_mir.server.the_display();
    auto const display_config = display->configuration();

    std::vector<geom::Rectangle> outputs;

     display_config->for_each_output([&] (mg::UserDisplayConfigurationOutput& output)
        {
            outputs.push_back(
                geom::Rectangle{
                    output.top_left,
                    output.modes[output.current_mode_index].size});
        });

    EXPECT_THAT(outputs, ContainerEq(display_geometry));
}

TEST_F(NestedServer, shell_sees_set_scaling_factor)
{
    NestedMirRunner nested_mir{new_connection()};

    constexpr float expected_scale{2.3f};
    constexpr MirFormFactor expected_form_factor{mir_form_factor_tv};

    change_display_configuration(nested_mir, expected_scale, expected_form_factor);
    auto const config_applied = wait_for_display_configuration_change(nested_mir, expected_scale, expected_form_factor);

    EXPECT_TRUE(config_applied);
}

TEST_F(NestedServer, client_sees_set_scaling_factor)
{
    NestedMirRunner nested_mir{new_connection()};

    constexpr float expected_scale{2.3f};
    constexpr MirFormFactor expected_form_factor{mir_form_factor_tv};

    change_display_configuration(nested_mir, expected_scale, expected_form_factor);
    auto const config_applied = wait_for_display_configuration_change(nested_mir, expected_scale, expected_form_factor);
    EXPECT_TRUE(config_applied);

    Client client{nested_mir};

    auto spec = mir_create_normal_window_spec(client.connection, 800, 600);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
#pragma GCC diagnostic pop
    mt::Signal surface_event_received;
    mir_window_spec_set_event_handler(spec, [](MirWindow*, MirEvent const* event, void* ctx)
        {
            if (mir_event_get_type(event) == mir_event_type_window_output)
            {
                auto surface_event = mir_event_get_window_output_event(event);
                EXPECT_THAT(mir_window_output_event_get_form_factor(surface_event), Eq(expected_form_factor));
                EXPECT_THAT(mir_window_output_event_get_scale(surface_event), Eq(expected_scale));
                auto signal = static_cast<mt::Signal*>(ctx);
                signal->raise();
            }
        },
        &surface_event_received);

    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_TRUE(surface_event_received.wait_for(30s));

    mir_window_release_sync(window);
}

//////////////////////////////////////////////////////////////////
// TODO the following test was used in investigating lifetime issues.
// TODO it may not have much long term value, but decide that later.
TEST_F(NestedServer, on_exit_display_objects_should_be_destroyed)
{
    std::weak_ptr<mir::graphics::Display> my_display;

    {
        NestedMirRunner nested_mir{new_connection()};

        my_display = nested_mir.server.the_display();
    }

    EXPECT_FALSE(my_display.lock()) << "after run_mir() exits the display should be released";
}

TEST_F(NestedServer, receives_lifecycle_events_from_host)
{
    NestedMirRunner nested_mir{new_connection()};

    mir::test::Signal events_processed;

    InSequence seq;
    EXPECT_CALL(*(nested_mir.the_mock_host_lifecycle_event_listener()),
        lifecycle_event_occurred(mir_lifecycle_state_resumed)).Times(1);
    EXPECT_CALL(*(nested_mir.the_mock_host_lifecycle_event_listener()),
        lifecycle_event_occurred(mir_lifecycle_state_will_suspend))
        .WillOnce(WakeUp(&events_processed));
    EXPECT_CALL(*(nested_mir.the_mock_host_lifecycle_event_listener()),
        lifecycle_event_occurred(mir_lifecycle_connection_lost)).Times(AtMost(1));

    trigger_lifecycle_event(mir_lifecycle_state_resumed);
    trigger_lifecycle_event(mir_lifecycle_state_will_suspend);

    events_processed.wait_for(timeout);
}

TEST_F(NestedServer, client_may_connect_to_nested_server_and_create_surface)
{
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurface client(nested_mir);

    bool became_exposed_and_focused = mir::test::spin_wait_for_condition_or_timeout(
        [window = client.window]
        {
            return mir_window_get_visibility(window) == mir_window_visibility_exposed
                && mir_window_get_focus_state(window) == mir_window_focus_state_focused;
        },
        timeout);

    EXPECT_TRUE(became_exposed_and_focused);
}

TEST_F(NestedServerWithTwoDisplays, DISABLED_posts_when_scene_has_visible_changes)
{
    auto const number_of_nested_surfaces = 2;
    auto const number_of_cursor_streams = number_of_nested_surfaces;
    // No post on window creation for the display surfaces - but preparing the cursor is fine
    EXPECT_CALL(*mock_session_mediator_report, session_submit_buffer_called(_)).Times(number_of_cursor_streams);
    NestedMirRunner nested_mir{new_connection()};

    auto const connection = mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__);
    auto const window = mtf::make_any_surface(connection);

    // NB there is no synchronization to guarantee that a spurious post on window creation will have
    // been seen by this point (although in testing it was invariably the case). However, any missed post
    // would be included in one of the later counts and cause a test failure.
    Mock::VerifyAndClearExpectations(mock_session_mediator_report.get());

    // One post on each output when window drawn
    {
        mt::Signal wait;

        EXPECT_CALL(*mock_session_mediator_report, session_submit_buffer_called(_)).Times(number_of_nested_surfaces)
            .WillOnce(InvokeWithoutArgs([]{}))
            .WillOnce(InvokeWithoutArgs([&] { wait.raise(); }));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop

        wait.wait_for(timeout);
        Mock::VerifyAndClearExpectations(mock_session_mediator_report.get());
    }

    // One post on each output when window released
    {
        mt::Signal wait;

        EXPECT_CALL(*mock_session_mediator_report, session_submit_buffer_called(_)).Times(number_of_nested_surfaces)
            .WillOnce(InvokeWithoutArgs([]{}))
            .WillOnce(InvokeWithoutArgs([&] { wait.raise(); }));

        mir_window_release_sync(window);
        mir_connection_release(connection);

        wait.wait_for(timeout);
        Mock::VerifyAndClearExpectations(mock_session_mediator_report.get());
    }

    // No post during shutdown
    EXPECT_CALL(*mock_session_mediator_report, session_submit_buffer_called(_)).Times(0);

    // Ignore other shutdown events
    EXPECT_CALL(*mock_session_mediator_report, session_release_surface_called(_)).Times(AnyNumber());
    EXPECT_CALL(*mock_session_mediator_report, session_disconnect_called(_)).Times(AnyNumber());
}

TEST_F(NestedServer, display_configuration_changes_are_forwarded_to_host)
{
    NestedMirRunner nested_mir{new_connection()};
    ClientWithAPaintedSurface client(nested_mir);
    ignore_rebuild_of_egl_context();

    mt::Signal condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), session_configuration_applied(_, _))
        .WillRepeatedly(InvokeWithoutArgs([&] { condition.raise(); }));

    client.update_display_configuration(
        [](MirDisplayConfig* config) {
            auto output = mir_display_config_get_mutable_output(config, 0);
            mir_output_disable(output);
    });

    ASSERT_TRUE(condition.wait_for(timeout));
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

TEST_F(NestedServer, display_orientation_changes_are_forwarded_to_host)
{
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurface client(nested_mir);

    auto const configuration = mir_connection_create_display_configuration(client.connection);

    for (auto new_orientation :
        {mir_orientation_left, mir_orientation_right, mir_orientation_inverted, mir_orientation_normal,
         mir_orientation_inverted, mir_orientation_right, mir_orientation_left, mir_orientation_normal})
    {
        // Allow for the egl context getting rebuilt as a side-effect each iteration
        ignore_rebuild_of_egl_context();

        mt::Signal config_reported;

        size_t num_outputs = mir_display_config_get_num_outputs(configuration);
        for (auto i = 0u; i < num_outputs; i++)
        {
            auto output = mir_display_config_get_mutable_output(configuration, i);
            mir_output_set_orientation(output, new_orientation);
        }

        EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(Pointee(mt::DisplayConfigMatches(configuration))))
            .WillRepeatedly(InvokeWithoutArgs([&] { config_reported.raise(); }));

        mir_connection_apply_session_display_config(client.connection, configuration);

        config_reported.wait_for(timeout);
        Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
    }

    mir_display_config_release(configuration);
}

TEST_F(NestedServer, animated_cursor_image_changes_are_forwarded_to_host)
{
    int const frames = 10;
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurfaceAndABufferStream client(nested_mir);
    nested_mir.wait_until_surface_ready(client.window);

    auto const mock_cursor = the_mock_cursor();

    server.the_cursor_listener()->cursor_moved_to(489, 9);

    // FIXME: In this test setup the software cursor will trigger scene_changed() on show(...).
    // Thus a new frame will be composed. Then a "FramePostObserver" in basic_surface.cpp will
    // react to the frame_posted callback by setting the cursor buffer again via show(..)
    // The number of show calls depends solely on scheduling decisions
    EXPECT_CALL(*mock_cursor, show(_)).Times(AtLeast(frames))
        .WillRepeatedly(InvokeWithoutArgs(
                    [&]
                    {
                        condition.raise();
                        test_processed_result.wait_for(timeout);
                        test_processed_result.reset();
                    }));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto conf = mir_cursor_configuration_from_buffer_stream(client.buffer_stream, 0, 0);
    mir_window_configure_cursor(client.window, conf);
    mir_cursor_configuration_destroy(conf);
#pragma GCC diagnostic pop

    EXPECT_TRUE(condition.wait_for(timeout));
    condition.reset();
    test_processed_result.raise();

    for (int i = 0; i != frames; ++i)
    {
        mir_buffer_stream_swap_buffers_sync(client.buffer_stream);

        EXPECT_TRUE(condition.wait_for(timeout));
        condition.reset();
        test_processed_result.raise();
    }
    Mock::VerifyAndClearExpectations(mock_cursor.get());
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(NestedServer, animated_cursor_image_changes_from_surface_are_forwarded_to_host)
{
    int const frames = 10;
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedWindowAndASurface client(nested_mir);
    nested_mir.wait_until_surface_ready(client.window);

    auto const mock_cursor = the_mock_cursor();

    server.the_cursor_listener()->cursor_moved_to(489, 9);

    // FIXME: In this test setup the software cursor will trigger scene_changed() on show(...).
    // Thus a new frame will be composed. Then a "FramePostObserver" in basic_surface.cpp will
    // react to the frame_posted callback by setting the cursor buffer again via show(..)
    // The number of show calls depends solely on scheduling decisions
    EXPECT_CALL(*mock_cursor, show(_)).Times(AtLeast(frames))
        .WillRepeatedly(InvokeWithoutArgs(
                    [&]
                    {
                        condition.raise();
                        test_processed_result.wait_for(timeout);
                        test_processed_result.reset();
                    }));

    auto conf = mir_cursor_configuration_from_render_surface(client.surface, 0, 0);
    mir_window_configure_cursor(client.window, conf);
    mir_cursor_configuration_destroy(conf);

    EXPECT_TRUE(condition.wait_for(long_timeout));
    condition.reset();
    test_processed_result.raise();

    for (int i = 0; i != frames; ++i)
    {
        mir_buffer_stream_swap_buffers_sync(client.buffer_stream);

        EXPECT_TRUE(condition.wait_for(timeout));
        condition.reset();
        test_processed_result.raise();
    }
    Mock::VerifyAndClearExpectations(mock_cursor.get());
}
#pragma GCC diagnostic pop

TEST_F(NestedServer, named_cursor_image_changes_are_forwarded_to_host)
{
    auto const mock_cursor = the_mock_cursor();
    ON_CALL(*mock_cursor, show(_)).WillByDefault(WakeUp(&condition));
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurface client(nested_mir);
    nested_mir.wait_until_surface_ready(client.window);

    server.the_cursor_listener()->cursor_moved_to(489, 9);

    // wait for the initial cursor show call..
    EXPECT_TRUE(condition.wait_for(long_timeout));
    condition.reset();

    // FIXME: In this test setup the software cursor will trigger scene_changed() on show(...).
    // Thus a new frame will be composed. Then a "FramePostObserver" in basic_surface.cpp will
    // react to the frame_posted callback by setting the cursor buffer again via show(..)
    // The number of show calls depends solely on scheduling decisions
    EXPECT_CALL(*mock_cursor, show(_)).Times(AtLeast(cursor_names.size()))
            .WillRepeatedly(InvokeWithoutArgs(
                    [&]
                    {
                        condition.raise();
                        test_processed_result.wait_for(long_timeout);
                        test_processed_result.reset();
                    }));


    for (auto const name : cursor_names)
    {
        auto spec = mir_create_window_spec(client.connection);
        mir_window_spec_set_cursor_name(spec, name);
        mir_window_apply_spec(client.window, spec);
        mir_window_spec_release(spec);

        EXPECT_TRUE(condition.wait_for(long_timeout));
        condition.reset();
        test_processed_result.raise();
    }
    Mock::VerifyAndClearExpectations(mock_cursor.get());
}

TEST_F(NestedServer, can_hide_the_host_cursor)
{
    int const frames = 10;
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurfaceAndABufferStream client(nested_mir);
    auto const mock_cursor = the_mock_cursor();
    nested_mir.wait_until_surface_ready(client.window);

    server.the_cursor_listener()->cursor_moved_to(489, 9);

    Mock::VerifyAndClearExpectations(mock_cursor.get());

    // FIXME: In this test setup the software cursor will trigger scene_changed() on show(...).
    // Thus a new frame will be composed. Then a "FramePostObserver" in basic_surface.cpp will
    // react to the frame_posted callback by setting the cursor buffer again via show(..)
    // The number of show calls depends solely on scheduling decisions
    EXPECT_CALL(*mock_cursor, show(_)).Times(AtLeast(1))
        .WillOnce(mt::WakeUp(&condition));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto conf = mir_cursor_configuration_from_buffer_stream(client.buffer_stream, 0, 0);
    mir_window_configure_cursor(client.window, conf);
    mir_cursor_configuration_destroy(conf);
#pragma GCC diagnostic pop

    std::this_thread::sleep_for(500ms);
    condition.wait_for(timeout);
    Mock::VerifyAndClearExpectations(mock_cursor.get());

    nested_mir.cursor_wrapper->set_hidden(true);

    EXPECT_CALL(*mock_cursor, show(_)).Times(0);

    for (int i = 0; i != frames; ++i)
    {
        mir_buffer_stream_swap_buffers_sync(client.buffer_stream);
    }

    // Need to verify before test server teardown deletes the
    // window as the host cursor then reverts to default.
    Mock::VerifyAndClearExpectations(mock_cursor.get());
}

//LP: #1597717
TEST_F(NestedServer, showing_a_0x0_cursor_image_sets_disabled_cursor)
{
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurfaceAndABufferStream client(nested_mir);
    nested_mir.wait_until_surface_ready(client.window);
    auto const mock_cursor = the_mock_cursor();

    server.the_cursor_listener()->cursor_moved_to(489, 9);

    //see NamedCursor in qtmir
    struct EmptyImage : mg::CursorImage
    {
        const void *as_argb_8888() const override { return nullptr; }
        geom::Size size() const override { return {0,0}; }
        geom::Displacement hotspot() const override { return {0,0}; }
    } empty_image;
    nested_mir.cursor_wrapper->show(empty_image);

    EXPECT_TRUE(stub_observer->wait_for_removal());

    // Need to verify before test server teardown deletes the
    // window as the host cursor then reverts to default.
    Mock::VerifyAndClearExpectations(mock_cursor.get());
}

TEST_F(NestedServer, applies_display_config_on_startup)
{
    mt::Signal condition;

    auto const expected_config = server.the_display()->configuration();
    expected_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
        { output.orientation = mir_orientation_inverted;});

    EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(Pointee(mt::DisplayConfigMatches(std::ref(*expected_config)))))
        .WillRepeatedly(InvokeWithoutArgs([&] { condition.raise(); }));

    struct MyNestedMirRunner : NestedMirRunner
    {
        MyNestedMirRunner(std::string const& connection_string) :
            NestedMirRunner(connection_string, true)
        {
            start_server();
        }

        std::shared_ptr<MockDisplayConfigurationPolicy> mock_display_configuration_policy() override
        {
            auto result = std::make_unique<MockDisplayConfigurationPolicy>();
            EXPECT_CALL(*result, apply_to(_)).Times(AnyNumber())
                .WillOnce(Invoke([](mg::DisplayConfiguration& config)
                     {
                        config.for_each_output([](mg::UserDisplayConfigurationOutput& output)
                            { output.orientation = mir_orientation_inverted; });
                     }));

            return std::shared_ptr<MockDisplayConfigurationPolicy>(std::move(result));
        }
    } nested_mir{new_connection()};

    ClientWithAPaintedSurface client(nested_mir);

    condition.wait_for(timeout);
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());

    EXPECT_TRUE(condition.raised());
}

TEST_F(NestedServer, base_configuration_change_in_host_is_seen_in_nested)
{
    NestedMirRunner nested_mir{new_connection()};
    ClientWithAPaintedSurface client(nested_mir);

    auto const config_policy = nested_mir.mock_display_configuration_policy();
    std::shared_ptr<mg::DisplayConfiguration> const new_config{server.the_display()->configuration()};
    new_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                    { output.orientation = mir_orientation_inverted;});

    mt::Signal condition;
    EXPECT_CALL(*config_policy, apply_to(mt::DisplayConfigMatches(std::ref(*new_config))))
        .WillOnce(InvokeWithoutArgs([&] { condition.raise(); }));

    server.the_display_configuration_controller()->set_base_configuration(new_config);

    condition.wait_for(timeout);
    Mock::VerifyAndClearExpectations(config_policy.get());
    EXPECT_TRUE(condition.raised());
}

// lp:1511798
TEST_F(NestedServerWithTwoDisplays, DISABLED_display_configuration_reset_when_application_exits)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    mt::Signal condition;
    {
        ClientWithAPaintedSurface client(nested_mir);

        {
            mt::Signal initial_condition;

            EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(_))
                .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.raise(); }));


            client.update_display_configuration(
                [](MirDisplayConfig* config) {
                    auto output = mir_display_config_get_mutable_output(config, 0);
                    mir_output_disable(output);
            });

            // Wait for initial config to be applied
            initial_condition.wait_for(timeout);

            Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
            ASSERT_TRUE(initial_condition.raised());
        }

        EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(_))
            .WillRepeatedly(InvokeWithoutArgs([&] { condition.raise(); }));
    }

    condition.wait_for(timeout);
    EXPECT_TRUE(condition.raised());
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

// lp:1511798
TEST_F(NestedServer, display_configuration_reset_when_nested_server_exits)
{
    mt::Signal condition;

    {
        NestedMirRunner nested_mir{new_connection()};
        ignore_rebuild_of_egl_context();

        ClientWithAPaintedSurface client(nested_mir);

        std::shared_ptr<mg::DisplayConfiguration> const new_config{nested_mir.server.the_display()->configuration()};
        new_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                        { output.orientation = mir_orientation_inverted;});

        mt::Signal initial_condition;

        EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(_))
            .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.raise(); }));

        nested_mir.server.the_display_configuration_controller()->set_base_configuration(new_config);

        // Wait for initial config to be applied
        initial_condition.wait_for(timeout);
        Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
        ASSERT_TRUE(initial_condition.raised());

        EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(_))
            .WillRepeatedly(InvokeWithoutArgs([&] { condition.raise(); }));
    }

    condition.wait_for(timeout);
    EXPECT_TRUE(condition.raised());
}

TEST_F(NestedServer, when_monitor_unplugs_client_is_notified_of_new_display_configuration)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    mt::Signal client_config_changed;
    ClientWithADisplayChangeCallbackAndAPaintedSurface client(
        nested_mir,
        [](MirConnection*, void* context) { static_cast<mt::Signal*>(context)->raise(); },
        &client_config_changed);

    auto const new_config = hw_display_config_for_unplug();

    display.emit_configuration_change_event(new_config);

    client_config_changed.wait_for(timeout);

    ASSERT_TRUE(client_config_changed.raised());

    auto const configuration = mir_connection_create_display_configuration(client.connection);

    EXPECT_THAT(configuration, mt::DisplayConfigMatches(*new_config));

    mir_display_config_release(configuration);
}

TEST_F(NestedServer, given_nested_server_set_base_display_configuration_when_monitor_unplugs_configuration_is_reset)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    /* We need to attach a painted client before attempting to set the display mode,
     * otherwise the nested server will not have rendered anything, so won't be focused,
     * so the host server won't apply any set configuration
     */
    ClientWithAPaintedSurface client(nested_mir);

    {
        std::shared_ptr<mg::DisplayConfiguration> const initial_config{nested_mir.server.the_display()->configuration()};
        initial_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                            { output.orientation = mir_orientation_inverted;});

        mt::Signal initial_condition;

        EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(_))
            .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.raise(); }));

        nested_mir.server.the_display_configuration_controller()->set_base_configuration(initial_config);

        // Wait for initial config to be applied
        initial_condition.wait_for(timeout);
        Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
        ASSERT_TRUE(initial_condition.raised());
    }

    auto const expect_config = hw_display_config_for_unplug();

    mt::Signal condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(Pointee(mt::DisplayConfigMatches(*expect_config))))
        .WillOnce(InvokeWithoutArgs([&] { condition.raise(); }));

    display.emit_configuration_change_event(expect_config);

    condition.wait_for(timeout);
    EXPECT_TRUE(condition.raised());
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

// TODO this test needs some core changes before it will pass. C.f. lp:1522802
// Specifically, ms::MediatingDisplayChanger::configure_for_hardware_change()
// doesn't reset the session config of the active client (even though it
// clears it from MediatingDisplayChanger::config_map).
// This is intentional, to avoid redundant reconfigurations, but we should
// handle the case of a client failing to apply a new config.
TEST_F(NestedServer, DISABLED_given_client_set_display_configuration_when_monitor_unplugs_configuration_is_reset)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    ClientWithAPaintedSurface client(nested_mir);

    client.update_display_configuration(
        [](MirDisplayConfig* config) {
            auto output = mir_display_config_get_mutable_output(config, 0);
            mir_output_disable(output);
    });

    auto const expect_config = hw_display_config_for_unplug();

    mt::Signal condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(Pointee(mt::DisplayConfigMatches(*expect_config))))
        .WillOnce(InvokeWithoutArgs([&] { condition.raise(); }));

    display.emit_configuration_change_event(expect_config);

    condition.wait_for(timeout);
    EXPECT_TRUE(condition.raised());
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

TEST_F(NestedServer, when_monitor_plugged_in_client_is_notified_of_new_display_configuration)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    mt::Signal client_config_changed;
    ClientWithADisplayChangeCallbackAndAPaintedSurface client(
        nested_mir,
        [](MirConnection*, void* context) { static_cast<mt::Signal*>(context)->raise(); },
        &client_config_changed);

    auto const new_config = hw_display_config_for_plugin();

    display.emit_configuration_change_event(new_config);

    client_config_changed.wait_for(timeout);

    ASSERT_TRUE(client_config_changed.raised());

    // The default layout policy (for cloned displays) will be applied by the MediatingDisplayChanger.
    // So set the expectation to match
    mtd::StubDisplayConfig expected_config(*new_config);
    expected_config.for_each_output([](mg::UserDisplayConfigurationOutput& output)
        { output.top_left = {0, 0}; });

    auto const configuration = mir_connection_create_display_configuration(client.connection);

    EXPECT_THAT(configuration, mt::DisplayConfigMatches(expected_config));

    mir_display_config_release(configuration);
}

TEST_F(NestedServer, DISABLED_given_nested_server_set_base_display_configuration_when_monitor_plugged_in_configuration_is_reset)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    /* We need to attach a painted client before attempting to set the display mode,
     * otherwise the nested server will not have rendered anything, so won't be focused,
     * so the host server won't apply any set configuration
     */
    ClientWithAPaintedSurface client(nested_mir);

    {
        std::shared_ptr<mg::DisplayConfiguration> const initial_config{nested_mir.server.the_display()->configuration()};
        initial_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                            { output.orientation = mir_orientation_inverted;});

        mt::Signal initial_condition;

        EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(_))
            .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.raise(); }));

        nested_mir.server.the_display_configuration_controller()->set_base_configuration(initial_config);

        // Wait for initial config to be applied
        initial_condition.wait_for(timeout);
        Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
        ASSERT_TRUE(initial_condition.raised());
    }

    auto const new_config = hw_display_config_for_plugin();

    // The default layout policy (for cloned displays) will be applied by the MediatingDisplayChanger.
    // So set the expectation to match
    mtd::StubDisplayConfig expected_config(*new_config);
    expected_config.for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                        { output.top_left = {0, 0}; });

    mt::Signal condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(Pointee(mt::DisplayConfigMatches(expected_config))))
        .WillOnce(InvokeWithoutArgs([&] { condition.raise(); }));

    display.emit_configuration_change_event(new_config);

    condition.wait_for(timeout);
    EXPECT_TRUE(condition.raised());
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

// TODO this test needs some core changes before it will pass. C.f. lp:1522802
// Specifically, ms::MediatingDisplayChanger::configure_for_hardware_change()
// doesn't reset the session config of the active client (even though it
// clears it from MediatingDisplayChanger::config_map).
// This is intentional, to avoid redundant reconfigurations, but we should
// handle the case of a client failing to apply a new config.
TEST_F(NestedServer, DISABLED_given_client_set_display_configuration_when_monitor_plugged_in_configuration_is_reset)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    ClientWithAPaintedSurface client(nested_mir);

    client.update_display_configuration(
        [](MirDisplayConfig* config) {
            auto output = mir_display_config_get_mutable_output(config, 0);
            mir_output_disable(output);
    });

    auto const new_config = hw_display_config_for_plugin();

    // The default layout policy (for cloned displays) will be applied by the MediatingDisplayChanger.
    // So set the expectation to match
    mtd::StubDisplayConfig expected_config(*new_config);
    expected_config.for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                        { output.top_left = {0, 0}; });

    mt::Signal condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(Pointee(mt::DisplayConfigMatches(expected_config))))
        .WillOnce(InvokeWithoutArgs([&] { condition.raise(); }));

    display.emit_configuration_change_event(new_config);

    condition.wait_for(timeout);
    EXPECT_TRUE(condition.raised());
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

TEST_F(NestedServerWithTwoDisplays,
    given_client_set_display_configuration_when_monitor_unplugs_client_is_notified_of_new_display_configuration)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    mt::Signal condition;

    ClientWithADisplayChangeCallbackAndAPaintedSurface client(
        nested_mir,
        [](MirConnection*, void* context) { static_cast<mt::Signal*>(context)->raise(); },
        &condition);

    mt::Signal initial_condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), configuration_applied(_))
        .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.raise(); }));

    client.update_display_configuration(
        [](MirDisplayConfig* config) {
            auto output = mir_display_config_get_mutable_output(config, 0);
            mir_output_disable(output);
    });

    initial_condition.wait_for(timeout);
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
    ASSERT_TRUE(initial_condition.raised());

    auto const new_config = hw_display_config_for_unplug();

    display.emit_configuration_change_event(new_config);

    condition.wait_for(timeout);

    EXPECT_TRUE(condition.raised());

    auto const configuration = mir_connection_create_display_configuration(client.connection);

    EXPECT_THAT(configuration, mt::DisplayConfigMatches(*new_config));

    mir_display_config_release(configuration);
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

TEST_F(NestedServer,
       given_client_set_display_configuration_when_monitor_unplugs_client_can_set_display_configuration)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    mt::Signal client_config_changed;

    ClientWithADisplayChangeCallbackAndAPaintedSurface client(
        nested_mir,
        [](MirConnection*, void* context) { static_cast<mt::Signal*>(context)->raise(); },
        &client_config_changed);

    client.update_display_configuration_applied_to(display,
        [](MirDisplayConfig* config) {
            auto output = mir_display_config_get_mutable_output(config, 0);
            mir_output_enable(output);
    });

    auto const new_hw_config = hw_display_config_for_unplug();

    auto const expected_config = std::make_shared<mtd::StubDisplayConfig>(*new_hw_config);
    expected_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
        { output.orientation = mir_orientation_inverted; });

    mt::Signal host_config_change;

    EXPECT_CALL(display, configure(Not(mt::DisplayConfigMatches(*expected_config)))).Times(AnyNumber())
        .WillRepeatedly(InvokeWithoutArgs([] {}));
    EXPECT_CALL(display, configure(mt::DisplayConfigMatches(*expected_config)))
        .WillRepeatedly(InvokeWithoutArgs([&] { host_config_change.raise(); }));

    display.emit_configuration_change_event(new_hw_config);

    client_config_changed.wait_for(timeout);
    if (client_config_changed.raised())
    {
        client.update_display_configuration(
            [](MirDisplayConfig* config) {

                auto output = mir_display_config_get_mutable_output(config, 0);
                mir_output_set_orientation(output, mir_orientation_inverted);
        });
    }

    host_config_change.wait_for(timeout);

    EXPECT_TRUE(host_config_change.raised());
    Mock::VerifyAndClearExpectations(&display);
}

TEST_F(NestedServerWithTwoDisplays, DISABLED_uses_passthrough_when_surface_size_is_appropriate)
{
    using namespace std::chrono_literals;
    NestedMirRunner nested_mir{new_connection()};
    ClientWithAPaintedSurface client(
        nested_mir, display_geometry.front().size, mir_pixel_format_xbgr_8888);
    nested_mir.wait_until_surface_ready(client.window);
    EXPECT_TRUE(nested_mir.passthrough_tracker->wait_for_passthrough_frames(1, 5s));
}
