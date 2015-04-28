/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/connected_client_with_a_surface.h"

#include "mir/events/event_builders.h"
#include "mir/shell/shell_wrapper.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"

#include "mir_test/fake_shared.h"
#include "mir_test/signal.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mev = mir::events;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mt = mir::test;

using namespace mir::geometry;
using namespace testing;

namespace
{
class MockSurfaceObserver : public ms::NullSurfaceObserver
{
public:
    MOCK_METHOD1(renamed, void(char const*));
    MOCK_METHOD1(resized_to, void(Size const& size));
};

struct StubShell : msh::ShellWrapper
{
    using msh::ShellWrapper::ShellWrapper;

    mf::SurfaceId create_surface(
        std::shared_ptr<ms::Session> const& session,
        ms::SurfaceCreationParameters const& params) override
    {
        auto const surface = msh::ShellWrapper::create_surface(session, params);
        latest_surface = session->surface(surface);
        return surface;
    }

    std::weak_ptr<ms::Surface> latest_surface;
};

struct SurfaceModifications : mtf::ConnectedClientWithASurface
{
    SurfaceModifications() { add_to_environment("MIR_SERVER_ENABLE_INPUT", "OFF"); }

    void SetUp() override
    {
        std::shared_ptr<StubShell> shell;

        server.wrap_shell([&](std::shared_ptr<msh::Shell> const& wrapped)
        {
            auto const msc = std::make_shared<StubShell>(wrapped);
            shell = msc;
            return msc;
        });

        mtf::ConnectedClientWithASurface::SetUp();

        shell_surface = shell->latest_surface;
        auto const scene_surface = shell_surface.lock();
        scene_surface->add_observer(mt::fake_shared(surface_observer));

        // Swap buffers to ensure surface is visible for event based tests
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    }

    void generate_alt_click_at(Point const& click_position)
    {
        MirInputDeviceId const device_id{7};
        int64_t const timestamp{39};
        auto const modifiers = mir_input_event_modifier_alt;
        std::vector<MirPointerButton> depressed_buttons{mir_pointer_button_tertiary};

        auto const x_axis_value = click_position.x.as_float();
        auto const y_axis_value = click_position.y.as_float();
        auto const hscroll_value = 0.0;
        auto const vscroll_value = 0.0;
        auto const action = mir_pointer_action_button_down;

        auto const click_event = mev::make_event(device_id, timestamp, modifiers,
            action, depressed_buttons, x_axis_value, y_axis_value, hscroll_value, vscroll_value);

        server.the_shell()->handle(*click_event);
    }

    void generate_alt_move_to(Point const& drag_position)
    {
        MirInputDeviceId const device_id{7};
        int64_t const timestamp{39};
        auto const modifiers = mir_input_event_modifier_alt;
        std::vector<MirPointerButton> depressed_buttons{mir_pointer_button_tertiary};

        auto const x_axis_value = drag_position.x.as_float();
        auto const y_axis_value = drag_position.y.as_float();
        auto const hscroll_value = 0.0;
        auto const vscroll_value = 0.0;
        auto const action = mir_pointer_action_motion;

        auto const drag_event = mev::make_event(device_id, timestamp, modifiers,
            action, depressed_buttons, x_axis_value, y_axis_value, hscroll_value, vscroll_value);

        server.the_shell()->handle(*drag_event);
    }

    void ensure_server_has_processed_setup()
    {
        mt::Signal server_ready;

        auto const new_title = __PRETTY_FUNCTION__;

        EXPECT_CALL(surface_observer, renamed(StrEq(new_title))).
            WillOnce(InvokeWithoutArgs([&]{ server_ready.raise(); }));

        auto const spec = mir_connection_create_spec_for_changes(connection);

        mir_surface_spec_set_name(spec, new_title);
        mir_surface_apply_spec(surface, spec);
        mir_surface_spec_release(spec);

        server_ready.wait();
    }

    MockSurfaceObserver surface_observer;
    std::weak_ptr<ms::Surface> shell_surface;
};

MATCHER_P(WidthEq, value, "")
{
    return Width(value) == arg.width;
}

MATCHER_P(HeightEq, value, "")
{
    return Height(value) == arg.height;
}

}

TEST_F(SurfaceModifications, surface_spec_name_is_notified)
{
    auto const new_title = __PRETTY_FUNCTION__;

    EXPECT_CALL(surface_observer, renamed(StrEq(new_title)));

    auto const spec = mir_connection_create_spec_for_changes(connection);

    mir_surface_spec_set_name(spec, new_title);
    mir_surface_apply_spec(surface, spec);
    mir_surface_spec_release(spec);
}

TEST_F(SurfaceModifications, surface_spec_resize_is_notified)
{
    auto const new_width = 5;
    auto const new_height = 7;

    EXPECT_CALL(surface_observer, resized_to(Size{new_width, new_height}));

    auto const spec = mir_connection_create_spec_for_changes(connection);

    mir_surface_spec_set_width(spec, new_width);
    mir_surface_spec_set_height(spec, new_height);

    mir_surface_apply_spec(surface, spec);
    mir_surface_spec_release(spec);
}

TEST_F(SurfaceModifications, surface_spec_change_width_is_notified)
{
    auto const new_width = 11;

    EXPECT_CALL(surface_observer, resized_to(WidthEq(new_width)));

    auto const spec = mir_connection_create_spec_for_changes(connection);

    mir_surface_spec_set_width(spec, new_width);

    mir_surface_apply_spec(surface, spec);
    mir_surface_spec_release(spec);
}

TEST_F(SurfaceModifications, surface_spec_change_height_is_notified)
{
    auto const new_height = 13;

    EXPECT_CALL(surface_observer, resized_to(HeightEq(new_height)));

    auto const spec = mir_connection_create_spec_for_changes(connection);

    mir_surface_spec_set_height(spec, new_height);

    mir_surface_apply_spec(surface, spec);
    mir_surface_spec_release(spec);
}

TEST_F(SurfaceModifications, surface_spec_min_width_is_respected)
{
    auto const min_width = 17;

    {
        auto const spec = mir_connection_create_spec_for_changes(connection);
        mir_surface_spec_set_min_width(spec, min_width);
        mir_surface_apply_spec(surface, spec);
        mir_surface_spec_release(spec);
    }

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, resized_to(WidthEq(min_width)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(shell_surface->top_left());
}

TEST_F(SurfaceModifications, surface_spec_min_height_is_respected)
{
    auto const min_height = 19;

    {
        auto const spec = mir_connection_create_spec_for_changes(connection);
        mir_surface_spec_set_min_height(spec, min_height);
        mir_surface_apply_spec(surface, spec);
        mir_surface_spec_release(spec);
    }

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, resized_to(HeightEq(min_height)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(shell_surface->top_left());
}

TEST_F(SurfaceModifications, surface_spec_max_width_is_respected)
{
    auto const max_width = 23;

    {
        auto const spec = mir_connection_create_spec_for_changes(connection);
        mir_surface_spec_set_max_width(spec, max_width);
        mir_surface_apply_spec(surface, spec);
        mir_surface_spec_release(spec);
    }

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, resized_to(WidthEq(max_width)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaX(max_width));
}

TEST_F(SurfaceModifications, surface_spec_max_height_is_respected)
{
    auto const max_height = 29;

    {
        auto const spec = mir_connection_create_spec_for_changes(connection);
        mir_surface_spec_set_max_height(spec, max_height);
        mir_surface_apply_spec(surface, spec);
        mir_surface_spec_release(spec);
    }

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, resized_to(HeightEq(max_height)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaY(max_height));
}
