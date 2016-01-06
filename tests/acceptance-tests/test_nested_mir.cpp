/*
 * Copyright © 2013-2015 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/frontend/session_mediator_report.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/cursor_image.h"
#include "mir/input/cursor_images.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_configuration.h"
#include "mir/graphics/display_configuration_policy.h"
#include "mir/graphics/display_configuration_report.h"
#include "mir/input/cursor_listener.h"
#include "mir/cached_ptr.h"
#include "mir/main_loop.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/session.h"
#include "mir/shell/display_configuration_controller.h"
#include "mir/shell/host_lifecycle_event_listener.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/headless_nested_server_runner.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/wait_condition.h"
#include "mir/test/spin_wait.h"
#include "mir/test/display_config_matchers.h"
#include "mir/test/doubles/fake_display.h"
#include "mir/test/doubles/stub_cursor.h"
#include "mir/test/doubles/stub_display_configuration.h"

#include "mir/test/doubles/nested_mock_egl.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace geom = mir::geometry;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

using namespace testing;
using namespace std::chrono_literals;

namespace
{
struct MockSessionMediatorReport : mf::SessionMediatorReport
{
    MockSessionMediatorReport()
    {
        EXPECT_CALL(*this, session_connect_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_disconnect_called(_)).Times(AnyNumber());

        // These are not needed for the 1st test, but they will be soon
        EXPECT_CALL(*this, session_create_surface_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_release_surface_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_next_buffer_called(_)).Times(AnyNumber());
        EXPECT_CALL(*this, session_submit_buffer_called(_)).Times(AnyNumber());
    }

    MOCK_METHOD1(session_connect_called, void (std::string const&));
    MOCK_METHOD1(session_create_surface_called, void (std::string const&));
    MOCK_METHOD1(session_next_buffer_called, void (std::string const&));
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

struct MockDisplayConfigurationReport : public mg::DisplayConfigurationReport
{
    MOCK_METHOD1(initial_configuration, void (mg::DisplayConfiguration const& configuration));
    MOCK_METHOD1(new_configuration, void (mg::DisplayConfiguration const& configuration));
};

std::vector<geom::Rectangle> const display_geometry
{
    {{  0, 0}, { 640,  480}},
    {{640, 0}, {1920, 1080}}
};

std::chrono::seconds const timeout{10};

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

class NestedMirRunner : public mtf::HeadlessNestedServerRunner
{
public:
    NestedMirRunner(std::string const& connection_string)
        : NestedMirRunner(connection_string, true)
    {
        start_server();
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
        return mock_display_configuration_policy_([this]
            { return std::make_shared<NiceMock<MockDisplayConfigurationPolicy>>(); });
    }

protected:
    NestedMirRunner(std::string const& connection_string, bool)
        : mtf::HeadlessNestedServerRunner(connection_string)
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
    mir::CachedPtr<MockHostLifecycleEventListener> mock_host_lifecycle_event_listener;
    
    mir::CachedPtr<MockDisplayConfigurationPolicy> mock_display_configuration_policy_;
};

struct NestedServer : mtf::HeadlessInProcessServer
{
    mtd::NestedMockEGL mock_egl;
    mtf::UsingStubClientPlatform using_stub_client_platform;

    std::shared_ptr<MockSessionMediatorReport> mock_session_mediator_report;
    NiceMock<MockDisplay> display{display_geometry};

    void SetUp() override
    {
        preset_display(mt::fake_shared(display));
        server.override_the_session_mediator_report([this]
            {
                mock_session_mediator_report = std::make_shared<NiceMock<MockSessionMediatorReport>>();
                return mock_session_mediator_report;
            });

        server.override_the_display_configuration_report([this]
            { return the_mock_display_configuration_report(); });

        server.wrap_cursor([this](std::shared_ptr<mg::Cursor> const&) { return the_mock_cursor(); });

        mtf::HeadlessInProcessServer::SetUp();
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
        return mock_display_configuration_report([]
            { return std::make_shared<NiceMock<MockDisplayConfigurationReport>>(); });
    }

    mir::CachedPtr<MockDisplayConfigurationReport> mock_display_configuration_report;

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
        auto new_displays = display_geometry;
        new_displays.resize(1);

        return std::make_shared<mtd::StubDisplayConfig>(new_displays);
    }


    auto hw_display_config_for_plugin() -> std::shared_ptr<mtd::StubDisplayConfig>
    {
        auto new_displays = display_geometry;
        new_displays.push_back({{2560, 0}, { 640,  480}});

        return  std::make_shared<mtd::StubDisplayConfig>(new_displays);
    }
};

struct Client
{
    explicit Client(NestedMirRunner& nested_mir) :
        connection(mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__))
    {}

    ~Client() { mir_connection_release(connection); }

    void update_display_configuration(void (*changer)(MirDisplayConfiguration* config))
    {
        auto const configuration = mir_connection_create_display_config(connection);
        changer(configuration);
        mir_wait_for(mir_connection_apply_display_config(connection, configuration));
        mir_display_config_destroy(configuration);
    }

    void update_display_configuration_applied_to(MockDisplay& display, void (*changer)(MirDisplayConfiguration* config))
    {
        mt::WaitCondition initial_condition;

        auto const configuration = mir_connection_create_display_config(connection);

        changer(configuration);

        EXPECT_CALL(display, configure(Not(mt::DisplayConfigMatches(configuration)))).Times(AnyNumber())
            .WillRepeatedly(InvokeWithoutArgs([] {}));
        EXPECT_CALL(display, configure(mt::DisplayConfigMatches(configuration)))
            .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.wake_up_everyone(); }));

        mir_wait_for(mir_connection_apply_display_config(connection, configuration));

        initial_condition.wait_for_at_most_seconds(1);
        mir_display_config_destroy(configuration);

        Mock::VerifyAndClearExpectations(&display);
        ASSERT_TRUE(initial_condition.woken());
    }

    MirConnection* const connection;
};

struct ClientWithADisplayChangeCallback : virtual Client
{
    ClientWithADisplayChangeCallback(NestedMirRunner& nested_mir, mir_display_config_callback callback, void* context) :
        Client(nested_mir)
    {
        mir_connection_set_display_config_change_callback(connection, callback, context);
    }
};

struct ClientWithAPaintedSurface : virtual Client
{
    ClientWithAPaintedSurface(NestedMirRunner& nested_mir) :
        Client(nested_mir),
        surface(mtf::make_any_surface(connection))
    {
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    }

    ~ClientWithAPaintedSurface()
    {
        mir_surface_release_sync(surface);
    }

    void update_surface_spec(void (*changer)(MirSurfaceSpec* spec))
    {
        auto const spec = mir_connection_create_spec_for_changes(connection);
        changer(spec);
        mir_surface_apply_spec(surface, spec);
        mir_surface_spec_release(spec);

    }

    MirSurface* const surface;
};

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

struct ClientWithADisplayChangeCallbackAndAPaintedSurface : virtual Client, ClientWithADisplayChangeCallback, ClientWithAPaintedSurface
{
    ClientWithADisplayChangeCallbackAndAPaintedSurface(NestedMirRunner& nested_mir, mir_display_config_callback callback, void* context) :
        Client(nested_mir),
        ClientWithADisplayChangeCallback(nested_mir, callback, context),
        ClientWithAPaintedSurface(nested_mir)
    {
    }
};
}

TEST_F(NestedServer, nested_platform_connects_and_disconnects)
{
    InSequence seq;
    EXPECT_CALL(*mock_session_mediator_report, session_connect_called(_)).Times(1);
    EXPECT_CALL(*mock_session_mediator_report, session_disconnect_called(_)).Times(1);

    NestedMirRunner{new_connection()};
}

TEST_F(NestedServer, sees_expected_outputs)
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

    mir::test::WaitCondition events_processed;

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

    events_processed.wait_for_at_most_seconds(5);
}

TEST_F(NestedServer, client_may_connect_to_nested_server_and_create_surface)
{
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurface client(nested_mir);

    bool became_exposed_and_focused = mir::test::spin_wait_for_condition_or_timeout(
        [surface = client.surface]
        {
            return mir_surface_get_visibility(surface) == mir_surface_visibility_exposed
                && mir_surface_get_focus(surface) == mir_surface_focused;
        },
        std::chrono::seconds{10});

    EXPECT_TRUE(became_exposed_and_focused);
}

TEST_F(NestedServer, posts_when_scene_has_visible_changes)
{
    // No post on surface creation
    EXPECT_CALL(*mock_session_mediator_report, session_submit_buffer_called(_)).Times(0);
    NestedMirRunner nested_mir{new_connection()};

    auto const connection = mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__);
    auto const surface = mtf::make_any_surface(connection);

    // NB there is no synchronization to guarantee that a spurious post on surface creation will have
    // been seen by this point (although in testing it was invariably the case). However, any missed post
    // would be included in one of the later counts and cause a test failure.
    Mock::VerifyAndClearExpectations(mock_session_mediator_report.get());

    // One post on each output when surface drawn
    {
        mt::WaitCondition wait;

        EXPECT_CALL(*mock_session_mediator_report, session_submit_buffer_called(_)).Times(2)
            .WillOnce(InvokeWithoutArgs([]{}))
            .WillOnce(InvokeWithoutArgs([&] { wait.wake_up_everyone(); }));

        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));

        wait.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(mock_session_mediator_report.get());
    }

    // One post on each output when surface released
    {
        mt::WaitCondition wait;

        EXPECT_CALL(*mock_session_mediator_report, session_submit_buffer_called(_)).Times(2)
            .WillOnce(InvokeWithoutArgs([]{}))
            .WillOnce(InvokeWithoutArgs([&] { wait.wake_up_everyone(); }));

        mir_surface_release_sync(surface);
        mir_connection_release(connection);

        wait.wait_for_at_most_seconds(1);
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

    mt::WaitCondition condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(_))
        .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

    client.update_display_configuration(
        [](MirDisplayConfiguration* config) { config->outputs->used = false; });

    condition.wait_for_at_most_seconds(1);
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

TEST_F(NestedServer, display_orientation_changes_are_forwarded_to_host)
{
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurface client(nested_mir);

    auto const configuration = mir_connection_create_display_config(client.connection);

    for (auto new_orientation :
        {mir_orientation_left, mir_orientation_right, mir_orientation_inverted, mir_orientation_normal,
         mir_orientation_inverted, mir_orientation_right, mir_orientation_left, mir_orientation_normal})
    {
        // Allow for the egl context getting rebuilt as a side-effect each iteration
        ignore_rebuild_of_egl_context();

        mt::WaitCondition condition;

        for(auto* output = configuration->outputs; output != configuration->outputs+configuration->num_outputs; ++ output)
            output->orientation = new_orientation;

        EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(mt::DisplayConfigMatches(configuration)))
            .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

        mir_wait_for(mir_connection_apply_display_config(client.connection, configuration));

        condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
    }

    mir_display_config_destroy(configuration);
}

TEST_F(NestedServer, animated_cursor_image_changes_are_forwarded_to_host)
{
    int const frames = 10;
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurfaceAndABufferStream client(nested_mir);
    auto const mock_cursor = the_mock_cursor();

    server.the_cursor_listener()->cursor_moved_to(489, 9);

    // TODO workaround for lp:1523621
    // (I don't see a way to detect that the host has placed focus on "Mir nested display for output #1")
    std::this_thread::sleep_for(10ms);

    {
        mt::WaitCondition condition;
        EXPECT_CALL(*mock_cursor, show(_)).Times(1)
            .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

        auto conf = mir_cursor_configuration_from_buffer_stream(client.buffer_stream, 0, 0);
        mir_wait_for(mir_surface_configure_cursor(client.surface, conf));
        mir_cursor_configuration_destroy(conf);

        condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(mock_cursor.get());
    }

    for (int i = 0; i != frames; ++i)
    {
        mt::WaitCondition condition;
        EXPECT_CALL(*mock_cursor, show(_)).Times(1)
            .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

        mir_buffer_stream_swap_buffers_sync(client.buffer_stream);

        condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(mock_cursor.get());
    }
}

TEST_F(NestedServer, named_cursor_image_changes_are_forwarded_to_host)
{
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurface client(nested_mir);
    auto const mock_cursor = the_mock_cursor();

    server.the_cursor_listener()->cursor_moved_to(489, 9);

    // TODO workaround for lp:1523621
    // (I don't see a way to detect that the host has placed focus on "Mir nested display for output #1")
    std::this_thread::sleep_for(10ms);

    for (auto const name : cursor_names)
    {
        mt::WaitCondition condition;

        EXPECT_CALL(*mock_cursor, show(_)).Times(1)
            .WillOnce(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

        auto const cursor = mir_cursor_configuration_from_name(name);
        mir_wait_for(mir_surface_configure_cursor(client.surface, cursor));
        mir_cursor_configuration_destroy(cursor);

        condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(mock_cursor.get());
    }
}

TEST_F(NestedServer, can_hide_the_host_cursor)
{
    int const frames = 10;
    NestedMirRunner nested_mir{new_connection()};

    ClientWithAPaintedSurfaceAndABufferStream client(nested_mir);
    auto const mock_cursor = the_mock_cursor();

    server.the_cursor_listener()->cursor_moved_to(489, 9);

    // TODO workaround for lp:1523621
    // (I don't see a way to detect that the host has placed focus on "Mir nested display for output #1")
    std::this_thread::sleep_for(10ms);

    {
        mt::WaitCondition condition;
        EXPECT_CALL(*mock_cursor, show(_)).Times(1)
            .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

        auto conf = mir_cursor_configuration_from_buffer_stream(client.buffer_stream, 0, 0);
        mir_wait_for(mir_surface_configure_cursor(client.surface, conf));
        mir_cursor_configuration_destroy(conf);

        condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(mock_cursor.get());
    }

    nested_mir.cursor_wrapper->set_hidden(true);

    EXPECT_CALL(*mock_cursor, show(_)).Times(0);

    for (int i = 0; i != frames; ++i)
    {
        mir_buffer_stream_swap_buffers_sync(client.buffer_stream);
    }

    // Need to verify before test server teardown deletes the
    // surface as the host cursor then reverts to default.
    Mock::VerifyAndClearExpectations(mock_cursor.get());
}

TEST_F(NestedServer, applies_display_config_on_startup)
{
    mt::WaitCondition condition;

    auto const expected_config = server.the_display()->configuration();
    expected_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
        { output.orientation = mir_orientation_inverted;});

    EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(mt::DisplayConfigMatches(std::ref(*expected_config))))
        .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

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

    condition.wait_for_at_most_seconds(1);
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());

    EXPECT_TRUE(condition.woken());
}

TEST_F(NestedServer, base_configuration_change_in_host_is_seen_in_nested)
{
    NestedMirRunner nested_mir{new_connection()};
    ClientWithAPaintedSurface client(nested_mir);

    auto const config_policy = nested_mir.mock_display_configuration_policy();
    std::shared_ptr<mg::DisplayConfiguration> const new_config{server.the_display()->configuration()};
    new_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                    { output.orientation = mir_orientation_inverted;});

    mt::WaitCondition condition;
    EXPECT_CALL(*config_policy, apply_to(mt::DisplayConfigMatches(std::ref(*new_config))))
        .WillOnce(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

    server.the_display_configuration_controller()->set_base_configuration(new_config);

    condition.wait_for_at_most_seconds(1);
    Mock::VerifyAndClearExpectations(config_policy.get());
    EXPECT_TRUE(condition.woken());
}

// lp:1511798
TEST_F(NestedServer, display_configuration_reset_when_application_exits)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    mt::WaitCondition condition;
    {
        ClientWithAPaintedSurface client(nested_mir);

        {
            mt::WaitCondition initial_condition;

            EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(_))
                .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.wake_up_everyone(); }));

            client.update_display_configuration(
                [](MirDisplayConfiguration* config) { config->outputs->used = false; });

            // Wait for initial config to be applied
            initial_condition.wait_for_at_most_seconds(1);

            Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
            ASSERT_TRUE(initial_condition.woken());
        }

        EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(_))
            .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));
    }

    condition.wait_for_at_most_seconds(1);
    EXPECT_TRUE(condition.woken());
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

// lp:1511798
TEST_F(NestedServer, display_configuration_reset_when_nested_server_exits)
{
    mt::WaitCondition condition;

    {
        NestedMirRunner nested_mir{new_connection()};
        ignore_rebuild_of_egl_context();

        ClientWithAPaintedSurface client(nested_mir);

        std::shared_ptr<mg::DisplayConfiguration> const new_config{nested_mir.server.the_display()->configuration()};
        new_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                        { output.orientation = mir_orientation_inverted;});

        mt::WaitCondition initial_condition;

        EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(_))
            .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.wake_up_everyone(); }));

        nested_mir.server.the_display_configuration_controller()->set_base_configuration(new_config);

        // Wait for initial config to be applied
        initial_condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
        ASSERT_TRUE(initial_condition.woken());

        EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(_))
            .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));
    }

    condition.wait_for_at_most_seconds(1);
    EXPECT_TRUE(condition.woken());
}

TEST_F(NestedServer, when_monitor_unplugs_client_is_notified_of_new_display_configuration)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    mt::WaitCondition client_config_changed;
    ClientWithADisplayChangeCallbackAndAPaintedSurface client(
        nested_mir,
        [](MirConnection*, void* context) { static_cast<mt::WaitCondition*>(context)->wake_up_everyone(); },
        &client_config_changed);

    auto const new_config = hw_display_config_for_unplug();

    display.emit_configuration_change_event(new_config);

    client_config_changed.wait_for_at_most_seconds(1);

    ASSERT_TRUE(client_config_changed.woken());

    auto const configuration = mir_connection_create_display_config(client.connection);

    EXPECT_THAT(configuration, mt::DisplayConfigMatches(*new_config));

    mir_display_config_destroy(configuration);
}

TEST_F(NestedServer, given_nested_server_set_base_display_configuration_when_monitor_unplugs_configuration_is_reset)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    {
        std::shared_ptr<mg::DisplayConfiguration> const initial_config{nested_mir.server.the_display()->configuration()};
        initial_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                            { output.orientation = mir_orientation_inverted;});

        mt::WaitCondition initial_condition;

        EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(_))
            .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.wake_up_everyone(); }));

        nested_mir.server.the_display_configuration_controller()->set_base_configuration(initial_config);

        // Wait for initial config to be applied
        initial_condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
        ASSERT_TRUE(initial_condition.woken());
    }

    ClientWithAPaintedSurface client(nested_mir);

    auto const expect_config = hw_display_config_for_unplug();

    mt::WaitCondition condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(mt::DisplayConfigMatches(*expect_config)))
        .WillOnce(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

    display.emit_configuration_change_event(expect_config);

    condition.wait_for_at_most_seconds(1);
    EXPECT_TRUE(condition.woken());
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

    client.update_display_configuration_applied_to(display,
        [](MirDisplayConfiguration* config) { config->outputs->used = false; });

    auto const expect_config = hw_display_config_for_unplug();

    mt::WaitCondition condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(mt::DisplayConfigMatches(*expect_config)))
        .WillOnce(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

    display.emit_configuration_change_event(expect_config);

    condition.wait_for_at_most_seconds(1);
    EXPECT_TRUE(condition.woken());
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

TEST_F(NestedServer, when_monitor_plugged_in_client_is_notified_of_new_display_configuration)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    mt::WaitCondition client_config_changed;
    ClientWithADisplayChangeCallbackAndAPaintedSurface client(
        nested_mir,
        [](MirConnection*, void* context) { static_cast<mt::WaitCondition*>(context)->wake_up_everyone(); },
        &client_config_changed);

    auto const new_config = hw_display_config_for_plugin();

    display.emit_configuration_change_event(new_config);

    client_config_changed.wait_for_at_most_seconds(1);

    ASSERT_TRUE(client_config_changed.woken());

    // The default layout policy (for cloned displays) will be applied by the MediatingDisplayChanger.
    // So set the expectation to match
    mtd::StubDisplayConfig expected_config(*new_config);
    expected_config.for_each_output([](mg::UserDisplayConfigurationOutput& output)
        { output.top_left = {0, 0}; });

    auto const configuration = mir_connection_create_display_config(client.connection);

    EXPECT_THAT(configuration, mt::DisplayConfigMatches(expected_config));

    mir_display_config_destroy(configuration);
}

TEST_F(NestedServer, given_nested_server_set_base_display_configuration_when_monitor_plugged_in_configuration_is_reset)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    {
        std::shared_ptr<mg::DisplayConfiguration> const initial_config{nested_mir.server.the_display()->configuration()};
        initial_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                            { output.orientation = mir_orientation_inverted;});

        mt::WaitCondition initial_condition;

        EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(_))
            .WillRepeatedly(InvokeWithoutArgs([&] { initial_condition.wake_up_everyone(); }));

        nested_mir.server.the_display_configuration_controller()->set_base_configuration(initial_config);

        // Wait for initial config to be applied
        initial_condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
        ASSERT_TRUE(initial_condition.woken());
    }

    ClientWithAPaintedSurface client(nested_mir);

    auto const new_config = hw_display_config_for_plugin();

    // The default layout policy (for cloned displays) will be applied by the MediatingDisplayChanger.
    // So set the expectation to match
    mtd::StubDisplayConfig expected_config(*new_config);
    expected_config.for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                        { output.top_left = {0, 0}; });

    mt::WaitCondition condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(mt::DisplayConfigMatches(expected_config)))
        .WillOnce(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

    display.emit_configuration_change_event(new_config);

    condition.wait_for_at_most_seconds(1);
    EXPECT_TRUE(condition.woken());
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

    client.update_display_configuration_applied_to(display,
        [](MirDisplayConfiguration* config) { config->outputs->used = false; });

    auto const new_config = hw_display_config_for_plugin();

    // The default layout policy (for cloned displays) will be applied by the MediatingDisplayChanger.
    // So set the expectation to match
    mtd::StubDisplayConfig expected_config(*new_config);
    expected_config.for_each_output([](mg::UserDisplayConfigurationOutput& output)
                                        { output.top_left = {0, 0}; });

    mt::WaitCondition condition;

    EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(mt::DisplayConfigMatches(expected_config)))
        .WillOnce(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

    display.emit_configuration_change_event(new_config);

    condition.wait_for_at_most_seconds(1);
    EXPECT_TRUE(condition.woken());
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

TEST_F(NestedServer,
    given_client_set_display_configuration_when_monitor_unplugs_client_is_notified_of_new_display_configuration)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    mt::WaitCondition condition;

    ClientWithADisplayChangeCallbackAndAPaintedSurface client(
        nested_mir,
        [](MirConnection*, void* context) { static_cast<mt::WaitCondition*>(context)->wake_up_everyone(); },
        &condition);

    client.update_display_configuration_applied_to(display,
        [](MirDisplayConfiguration* config) { config->outputs->used = false; });

    auto const new_config = hw_display_config_for_unplug();

    display.emit_configuration_change_event(new_config);

    condition.wait_for_at_most_seconds(1);

    EXPECT_TRUE(condition.woken());

    auto const configuration = mir_connection_create_display_config(client.connection);

    EXPECT_THAT(configuration, mt::DisplayConfigMatches(*new_config));

    mir_display_config_destroy(configuration);
    Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
}

TEST_F(NestedServer,
       given_client_set_display_configuration_when_monitor_unplugs_client_can_set_display_configuration)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    mt::WaitCondition client_config_changed;

    ClientWithADisplayChangeCallbackAndAPaintedSurface client(
        nested_mir,
        [](MirConnection*, void* context) { static_cast<mt::WaitCondition*>(context)->wake_up_everyone(); },
        &client_config_changed);

    client.update_display_configuration_applied_to(display,
        [](MirDisplayConfiguration* config) { config->outputs->used = mir_orientation_inverted; });

    auto const new_hw_config = hw_display_config_for_unplug();

    auto const expected_config = std::make_shared<mtd::StubDisplayConfig>(*new_hw_config);
    expected_config->for_each_output([](mg::UserDisplayConfigurationOutput& output)
        { output.orientation = mir_orientation_inverted; });

    mt::WaitCondition host_config_change;

    EXPECT_CALL(display, configure(Not(mt::DisplayConfigMatches(*expected_config)))).Times(AnyNumber())
        .WillRepeatedly(InvokeWithoutArgs([] {}));
    EXPECT_CALL(display, configure(mt::DisplayConfigMatches(*expected_config)))
        .WillRepeatedly(InvokeWithoutArgs([&] { host_config_change.wake_up_everyone(); }));

    display.emit_configuration_change_event(new_hw_config);

    client_config_changed.wait_for_at_most_seconds(1);
    if (client_config_changed.woken())
    {
        client.update_display_configuration(
            [](MirDisplayConfiguration* config) { config->outputs->orientation = mir_orientation_inverted; });
    }

    host_config_change.wait_for_at_most_seconds(1);

    EXPECT_TRUE(host_config_change.woken());
    Mock::VerifyAndClearExpectations(&display);
}
