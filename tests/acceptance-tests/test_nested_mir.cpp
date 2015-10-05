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
#include "mir/graphics/display_configuration_report.h"
#include "mir/input/cursor_listener.h"
#include "mir/cached_ptr.h"
#include "mir/main_loop.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/session.h"
#include "mir/shell/host_lifecycle_event_listener.h"

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/headless_nested_server_runner.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/wait_condition.h"
#include "mir/test/spin_wait.h"
#include "mir/test/display_config_matchers.h"
#include "mir/test/doubles/stub_cursor.h"

#include "mir/test/doubles/nested_mock_egl.h"

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
    {{480, 0}, {1920, 1080}}
};

std::chrono::seconds const timeout{10};

class NestedMirRunner : public mtf::HeadlessNestedServerRunner
{
public:
    NestedMirRunner(std::string const& connection_string)
        : mtf::HeadlessNestedServerRunner(connection_string)
    {
        server.override_the_host_lifecycle_event_listener([this]
            { return the_mock_host_lifecycle_event_listener(); });

        start_server();
    }

    ~NestedMirRunner()
    {
        stop_server();
    }

    std::shared_ptr<MockHostLifecycleEventListener> the_mock_host_lifecycle_event_listener()
    {
        return mock_host_lifecycle_event_listener([]
            { return std::make_shared<NiceMock<MockHostLifecycleEventListener>>(); });
    }

private:
    mir::CachedPtr<MockHostLifecycleEventListener> mock_host_lifecycle_event_listener;
};

struct NestedServer : mtf::HeadlessInProcessServer
{
    mtd::NestedMockEGL mock_egl;
    mtf::UsingStubClientPlatform using_stub_client_platform;

    std::shared_ptr<MockSessionMediatorReport> mock_session_mediator_report;

    void SetUp() override
    {
        initial_display_layout(display_geometry);
        server.override_the_session_mediator_report([this]
            {
                mock_session_mediator_report = std::make_shared<MockSessionMediatorReport>();
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

    MirSurface* make_and_paint_surface(MirConnection* connection) const
    {
        auto const surface = mtf::make_any_surface(connection);
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
        return surface;
    }

    void ignore_rebuild_of_egl_context()
    {
        InSequence context_lifecycle;
        EXPECT_CALL(mock_egl, eglCreateContext(_, _, _, _)).Times(AnyNumber()).WillRepeatedly(Return((EGLContext)this));
        EXPECT_CALL(mock_egl, eglDestroyContext(_, _)).Times(AnyNumber()).WillRepeatedly(Return(EGL_TRUE));
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

    auto c = mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__);
    auto surface = make_and_paint_surface(c);

    bool became_exposed_and_focused = mir::test::spin_wait_for_condition_or_timeout(
        [surface]
        {
            return mir_surface_get_visibility(surface) == mir_surface_visibility_exposed
                && mir_surface_get_focus(surface) == mir_surface_focused;
        },
        std::chrono::seconds{10});

    EXPECT_TRUE(became_exposed_and_focused);

    mir_surface_release_sync(surface);
    mir_connection_release(c);
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

    // One post when surface drawn
    {
        mt::WaitCondition wait;

        EXPECT_CALL(*mock_session_mediator_report, session_submit_buffer_called(_)).Times(1)
                .WillOnce(InvokeWithoutArgs([&] { wait.wake_up_everyone(); }));

        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));

        wait.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(mock_session_mediator_report.get());
    }

    // One post when surface released
    {
        mt::WaitCondition wait;

        EXPECT_CALL(*mock_session_mediator_report, session_submit_buffer_called(_)).Times(1)
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

    auto const connection = mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__);

    // Need a painted surface to have focus
    auto const painted_surface = make_and_paint_surface(connection);

    auto const configuration = mir_connection_create_display_config(connection);

    configuration->outputs->used = false;

    mt::WaitCondition condition;

    ignore_rebuild_of_egl_context();
    EXPECT_CALL(*the_mock_display_configuration_report(), new_configuration(_))
        .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

    mir_wait_for(mir_connection_apply_display_config(connection, configuration));

    condition.wait_for_at_most_seconds(1);

    mir_display_config_destroy(configuration);
    mir_surface_release_sync(painted_surface);
    mir_connection_release(connection);
}

TEST_F(NestedServer, display_orientation_changes_are_forwarded_to_host)
{
    NestedMirRunner nested_mir{new_connection()};

    auto const connection = mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__);

    // Need a painted surface to have focus
    auto const painted_surface = make_and_paint_surface(connection);

    auto const configuration = mir_connection_create_display_config(connection);

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

        mir_wait_for(mir_connection_apply_display_config(connection, configuration));

        condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(the_mock_display_configuration_report().get());
    }

    mir_display_config_destroy(configuration);
    mir_surface_release_sync(painted_surface);
    mir_connection_release(connection);
}

// lp:1491937
TEST_F(NestedServer, display_configuration_changes_are_visible_to_client)
{
    NestedMirRunner nested_mir{new_connection()};

    auto const connection = mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__);

    // Need a painted surface to have focus
    auto const painted_surface = make_and_paint_surface(connection);

    auto const configuration = mir_connection_create_display_config(connection);

    for (auto new_orientation :
        {mir_orientation_left, mir_orientation_right, mir_orientation_inverted, mir_orientation_normal,
         mir_orientation_inverted, mir_orientation_right, mir_orientation_left, mir_orientation_normal})
    {
        // Allow for the egl context getting rebuilt as a side-effect each iteration
        ignore_rebuild_of_egl_context();

        for(auto* output = configuration->outputs; output != configuration->outputs+configuration->num_outputs; ++ output)
            output->orientation = new_orientation;

        mir_wait_for(mir_connection_apply_display_config(connection, configuration));

        auto const new_config = mir_connection_create_display_config(connection);
        EXPECT_THAT(new_config->outputs->orientation, Eq(configuration->outputs->orientation));
        mir_display_config_destroy(new_config);
    }

    mir_display_config_destroy(configuration);
    mir_surface_release_sync(painted_surface);
    mir_connection_release(connection);
}

namespace
{
void config_update(MirConnection* /*connection*/, void* context)
{
    static_cast<mt::WaitCondition*>(context)->wake_up_everyone();
}
}

// lp:1493741
TEST_F(NestedServer, display_configuration_changes_are_visible_to_client_when_it_becomes_active)
{
    NestedMirRunner nested_mir{new_connection()};
    ignore_rebuild_of_egl_context();

    auto const connection = mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__);

    mt::WaitCondition condition;
    mir_connection_set_display_config_change_callback(connection, &config_update, &condition);

    auto const configuration = mir_connection_create_display_config(connection);

    for(auto* output = configuration->outputs; output != configuration->outputs+configuration->num_outputs; ++ output)
        output->orientation = mir_orientation_left;

    mir_wait_for(mir_connection_apply_display_config(connection, configuration));

    // Need a painted surface to have focus
    auto const painted_surface = make_and_paint_surface(connection);

    condition.wait_for_at_most_seconds(1);

    auto const new_config = mir_connection_create_display_config(connection);
    EXPECT_THAT(new_config->outputs->orientation, Eq(configuration->outputs->orientation));
    mir_display_config_destroy(new_config);

    mir_display_config_destroy(configuration);
    mir_surface_release_sync(painted_surface);
    mir_connection_release(connection);
}

TEST_F(NestedServer, animated_cursor_image_changes_are_forwarded_to_host)
{
    int const frames = 10;
    NestedMirRunner nested_mir{new_connection()};

    auto const connection = mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__);
    auto const surface = make_and_paint_surface(connection);
    auto const mock_cursor = the_mock_cursor();

    auto const spec = mir_connection_create_spec_for_changes(connection);
    mir_surface_spec_set_fullscreen_on_output(spec, 1);
    mir_surface_apply_spec(surface, spec);
    mir_surface_spec_release(spec);

    server.the_cursor_listener()->cursor_moved_to(489, 9);

    auto stream = mir_connection_create_buffer_stream_sync(
        connection, 24, 24, mir_pixel_format_argb_8888, mir_buffer_usage_software);
    mir_buffer_stream_swap_buffers_sync(stream);

    {
        mt::WaitCondition condition;
        EXPECT_CALL(*mock_cursor, show(_)).Times(1)
            .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

        auto conf = mir_cursor_configuration_from_buffer_stream(stream, 0, 0);
        mir_wait_for(mir_surface_configure_cursor(surface, conf));
        mir_cursor_configuration_destroy(conf);

        condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(mock_cursor.get());
    }

    for (int i = 0; i != frames; ++i)
    {
        mt::WaitCondition condition;
        EXPECT_CALL(*mock_cursor, show(_)).Times(1)
            .WillRepeatedly(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

        mir_buffer_stream_swap_buffers_sync(stream);

        condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(mock_cursor.get());
    }

    mir_buffer_stream_release_sync(stream);
    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

// We can't rely on the test environment to have X cursors, so we provide some of our own
namespace 
{
static auto const cursor_names = {
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

struct CursorImage : public mg::CursorImage
{
    CursorImage(char const* name) :
        id{std::find(begin(cursor_names), end(cursor_names), name) - begin(cursor_names)},
        data{{uint8_t(id)}}
    {
    }

    void const* as_argb_8888() const { return data.data(); }

    geom::Size size() const { return { 24, 24 }; }

    geom::Displacement hotspot() const { return {0, 0}; }

    ptrdiff_t id;
    std::array<uint8_t, 4*8*24*24> data;
};

struct CursorImages : mir::input::CursorImages
{
public:

    std::shared_ptr<mg::CursorImage> image(std::string const& cursor_name, geom::Size const& /*size*/)
    {
        return std::make_shared<CursorImage>(cursor_name.c_str());
    }
};

class NestedMirRunnerWithCursorImages : public mtf::HeadlessNestedServerRunner
{
public:
    NestedMirRunnerWithCursorImages(std::string const& connection_string)
        : mtf::HeadlessNestedServerRunner(connection_string)
    {
        server.override_the_cursor_images([] { return std::make_shared<CursorImages>(); });
        start_server();
    }

    ~NestedMirRunnerWithCursorImages()
    {
        stop_server();
    }
};
}

TEST_F(NestedServer, named_cursor_image_changes_are_forwarded_to_host)
{
    NestedMirRunnerWithCursorImages nested_mir{new_connection()};

    auto const connection = mir_connect_sync(nested_mir.new_connection().c_str(), __PRETTY_FUNCTION__);
    auto const surface = make_and_paint_surface(connection);
    auto const mock_cursor = the_mock_cursor();

    auto const spec = mir_connection_create_spec_for_changes(connection);
    mir_surface_spec_set_fullscreen_on_output(spec, 1);
    mir_surface_apply_spec(surface, spec);
    mir_surface_spec_release(spec);

    server.the_cursor_listener()->cursor_moved_to(489, 9);

    for (auto const name : cursor_names)
    {
        mt::WaitCondition condition;

        EXPECT_CALL(*mock_cursor, show(_)).Times(1)
            .WillOnce(InvokeWithoutArgs([&] { condition.wake_up_everyone(); }));

        auto const cursor = mir_cursor_configuration_from_name(name);
        mir_wait_for(mir_surface_configure_cursor(surface, cursor));
        mir_cursor_configuration_destroy(cursor);

        condition.wait_for_at_most_seconds(1);
        Mock::VerifyAndClearExpectations(mock_cursor.get());
    }

    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}
