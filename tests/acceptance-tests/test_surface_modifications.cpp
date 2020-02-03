/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/events/event_builders.h"
#include "mir/scene/surface.h"
#include "mir/scene/null_surface_observer.h"

#include "mir/test/doubles/wrap_shell_to_track_latest_surface.h"
#include "mir_test_framework/connected_client_with_a_window.h"
#include "mir/test/fake_shared.h"
#include "mir/test/signal.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mev = mir::events;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace mir::geometry;
using namespace testing;

using namespace std::literals::chrono_literals;

namespace
{
class MockSurfaceObserver : public ms::NullSurfaceObserver
{
public:
    MOCK_METHOD2(renamed, void(ms::Surface const*, char const*));
    MOCK_METHOD2(content_resized_to, void(ms::Surface const*, Size const& size));
    MOCK_METHOD2(hidden_set_to, void(ms::Surface const*, bool));
};

struct SurfaceModifications : mtf::ConnectedClientWithAWindow
{
    SurfaceModifications() { add_to_environment("MIR_SERVER_ENABLE_INPUT", "OFF"); }

    void SetUp() override
    {
        std::shared_ptr<mtd::WrapShellToTrackLatestSurface> shell;

        server.wrap_shell([&](std::shared_ptr<msh::Shell> const& wrapped)
        {
            auto const msc = std::make_shared<mtd::WrapShellToTrackLatestSurface>(wrapped);
            shell = msc;
            return msc;
        });

        mtf::ConnectedClientWithAWindow::SetUp();

        shell_surface = shell->latest_surface;
        auto const scene_surface = shell_surface.lock();
        scene_surface->add_observer(mt::fake_shared(surface_observer));

        // Swap buffers to ensure window is visible for event based tests
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop
    }

    void generate_alt_click_at(Point const& click_position)
    {
        auto const modifiers = mir_input_event_modifier_alt;

        auto const x_axis_value = click_position.x.as_int();
        auto const y_axis_value = click_position.y.as_int();
        auto const hscroll_value = 0.0;
        auto const vscroll_value = 0.0;
        auto const action = mir_pointer_action_button_down;
        auto const relative_x_value = 0.0;
        auto const relative_y_value = 0.0;

        auto const click_event = mev::make_event(device_id, timestamp, cookie, modifiers,
            action, mir_pointer_button_tertiary, x_axis_value, y_axis_value,
            hscroll_value, vscroll_value, relative_x_value, relative_y_value);

        server.the_shell()->handle(*click_event);
    }

    void generate_alt_move_to(Point const& drag_position)
    {
        auto const modifiers = mir_input_event_modifier_alt;

        auto const x_axis_value = drag_position.x.as_int();
        auto const y_axis_value = drag_position.y.as_int();
        auto const hscroll_value = 0.0;
        auto const vscroll_value = 0.0;
        auto const action = mir_pointer_action_motion;
        auto const relative_x_value = 0.0;
        auto const relative_y_value = 0.0;

        auto const drag_event = mev::make_event(device_id, timestamp, cookie, modifiers,
            action, mir_pointer_button_tertiary, x_axis_value, y_axis_value,
            hscroll_value, vscroll_value, relative_x_value, relative_y_value);

        server.the_shell()->handle(*drag_event);
    }

    void ensure_server_has_processed_setup()
    {
        mt::Signal server_ready;

        auto const new_title = __PRETTY_FUNCTION__;

        EXPECT_CALL(surface_observer, renamed(_, StrEq(new_title))).
            WillOnce(InvokeWithoutArgs([&]{ server_ready.raise(); }));

        apply_changes([&](MirWindowSpec* spec)
            {
                mir_window_spec_set_name(spec, new_title);
            });

        server_ready.wait();
    }

    template<typename Specifier>
    void apply_changes(Specifier const& specifier) const
    {
        auto const spec = mir_create_window_spec(connection);

        specifier(spec);

        mir_window_apply_spec(window, spec);
        mir_window_spec_release(spec);
    }

    MirInputDeviceId const device_id = MirInputDeviceId(7);
    std::chrono::nanoseconds const timestamp = std::chrono::nanoseconds(39);
    NiceMock<MockSurfaceObserver> surface_observer;
    std::weak_ptr<ms::Surface> shell_surface;
    std::vector<uint8_t> cookie;
};

MATCHER_P(WidthEq, value, "")
{
    return Width(value) == arg.width;
}

MATCHER_P(HeightEq, value, "")
{
    return Height(value) == arg.height;
}

struct StatePair
{
    MirWindowState from;
    MirWindowState to;

    friend std::ostream& operator<<(std::ostream& out, StatePair const& types)
        { return out << "from:" << types.from << ", to:" << types.to; }
};

bool is_visible(MirWindowState state)
{
    switch (state)
    {
    case mir_window_state_hidden:
    case mir_window_state_minimized:
        return false;
    default:
        break;
    }
    return true;
}

struct SurfaceStateCase : SurfaceModifications, ::testing::WithParamInterface<StatePair> {};
struct SurfaceSpecStateCase : SurfaceModifications, ::testing::WithParamInterface<StatePair> {};
}

TEST_F(SurfaceModifications, surface_spec_name_is_notified)
{
    auto const new_title = __PRETTY_FUNCTION__;

    EXPECT_CALL(surface_observer, renamed(_, StrEq(new_title)));

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_name(spec, new_title);
        });
}

TEST_F(SurfaceModifications, surface_spec_resize_is_notified)
{
    auto const new_width = 5;
    auto const new_height = 7;

    EXPECT_CALL(surface_observer, content_resized_to(_, Size{new_width, new_height}));

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_width(spec, new_width);
            mir_window_spec_set_height(spec, new_height);
        });
}

TEST_F(SurfaceModifications, surface_spec_change_width_is_notified)
{
    auto const new_width = 11;

    EXPECT_CALL(surface_observer, content_resized_to(_, WidthEq(new_width)));

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_width(spec, new_width);
        });
}

TEST_F(SurfaceModifications, surface_spec_change_height_is_notified)
{
    auto const new_height = 13;

    EXPECT_CALL(surface_observer, content_resized_to(_, HeightEq(new_height)));

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_height(spec, new_height);
        });
}

TEST_F(SurfaceModifications, surface_spec_min_width_is_respected)
{
    auto const min_width = 17;

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_min_width(spec, min_width);
        });

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, content_resized_to(_, WidthEq(min_width)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(shell_surface->top_left());
}

TEST_F(SurfaceModifications, surface_spec_min_height_is_respected)
{
    auto const min_height = 19;

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_min_height(spec, min_height);
        });

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, content_resized_to(_, HeightEq(min_height)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(shell_surface->top_left());
}

TEST_F(SurfaceModifications, surface_spec_max_width_is_respected)
{
    auto const max_width = 23;

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_max_width(spec, max_width);
        });

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, content_resized_to(_, WidthEq(max_width)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaX(max_width));
}

TEST_F(SurfaceModifications, surface_spec_max_height_is_respected)
{
    auto const max_height = 29;

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_max_height(spec, max_height);
        });

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    EXPECT_CALL(surface_observer, content_resized_to(_, HeightEq(max_height)));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaY(max_height));
}

TEST_F(SurfaceModifications, surface_spec_width_inc_is_respected)
{
    auto const width_inc = 13;

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_width_increment(spec, width_inc);
        });

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaX(16));

    EXPECT_TRUE(actual.width.as_int() % width_inc == 0);
}

TEST_F(SurfaceModifications, surface_spec_with_min_width_and_width_inc_is_respected)
{
    auto const width_inc = 13;
    auto const min_width = 7;

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_width_increment(spec, width_inc);
            mir_window_spec_set_min_width(spec, min_width);
        });

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaX(16));

    EXPECT_TRUE((actual.width.as_int() - min_width) % width_inc == 0);
}

TEST_F(SurfaceModifications, surface_spec_height_inc_is_respected)
{
    auto const height_inc = 13;

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_height_increment(spec, height_inc);
        });

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaY(16));

    EXPECT_TRUE(actual.height.as_int() % height_inc == 0);
}

TEST_F(SurfaceModifications, surface_spec_with_min_height_and_height_inc_is_respected)
{
    auto const height_inc = 13;
    auto const min_height = 7;

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_height_increment(spec, height_inc);
            mir_window_spec_set_min_height(spec, min_height);
        });

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_right + DeltaY(16));

    EXPECT_TRUE((actual.height.as_int() - min_height) % height_inc == 0);
}

TEST_F(SurfaceModifications, surface_spec_with_min_aspect_ratio_is_respected)
{
    auto const aspect_width = 11;
    auto const aspect_height = 7;

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_min_aspect_ratio(spec, aspect_width, aspect_height);
        });

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};
    auto const bottom_left = shell_surface->input_bounds().bottom_left() + Displacement{1,-1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(bottom_left);

    EXPECT_THAT(float(actual.width.as_int())/actual.height.as_int(), Ge(float(aspect_width)/aspect_height));
}

TEST_F(SurfaceModifications, surface_spec_with_max_aspect_ratio_is_respected)
{
    auto const aspect_width = 7;
    auto const aspect_height = 11;

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_max_aspect_ratio(spec, aspect_width, aspect_height);
        });

    ensure_server_has_processed_setup();

    auto const shell_surface = this->shell_surface.lock();
    auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};
    auto const top_right = shell_surface->input_bounds().top_right() - Displacement{1,-1};

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).WillOnce(SaveArg<1>(&actual));

    generate_alt_click_at(bottom_right);
    generate_alt_move_to(top_right);

    EXPECT_THAT(float(actual.width.as_int())/actual.height.as_int(), Le(float(aspect_width)/aspect_height));
}

TEST_F(SurfaceModifications, surface_spec_with_fixed_aspect_ratio_and_size_range_is_respected)
{
    auto const aspect_width = 11;
    auto const aspect_height = 7;
    auto const min_width = 10*aspect_width;
    auto const min_height = 10*aspect_height;
    auto const max_width = 20*aspect_width;
    auto const max_height = 20*aspect_height;
    auto const width_inc = 11;
    auto const height_inc = 7;

    Size actual;
    EXPECT_CALL(surface_observer, content_resized_to(_, _)).Times(AnyNumber()).WillRepeatedly(SaveArg<1>(&actual));

    apply_changes([&](MirWindowSpec* spec)
          {
              mir_window_spec_set_min_aspect_ratio(spec, aspect_width, aspect_height);
              mir_window_spec_set_max_aspect_ratio(spec, aspect_width, aspect_height);

              mir_window_spec_set_min_height(spec, min_height);
              mir_window_spec_set_min_width(spec, min_width);

              mir_window_spec_set_max_height(spec, max_height);
              mir_window_spec_set_max_width(spec, max_width);

              mir_window_spec_set_width_increment(spec, width_inc);
              mir_window_spec_set_height_increment(spec, height_inc);

              mir_window_spec_set_height(spec, min_height);
              mir_window_spec_set_width(spec, min_width);
          });

    ensure_server_has_processed_setup();

    auto const expected_aspect_ratio = FloatEq(float(aspect_width)/aspect_height);

    EXPECT_THAT(static_cast<float>(actual.width.as_int())/actual.height.as_int(), expected_aspect_ratio);
    EXPECT_THAT(actual, Eq(Size{min_width, min_height}));
    
    for (int delta = 1; delta != 20; ++delta)
    {
        auto const shell_surface = this->shell_surface.lock();
        auto const bottom_right = shell_surface->input_bounds().bottom_right() - Displacement{1,1};

        // Introduce small variation around "accurate" resize motion
        auto const jitter = Displacement{delta%2 ? +2 : -2, (delta/2)%2 ? +2 : -2};
        auto const motion = Displacement{width_inc, height_inc} + jitter;

        generate_alt_click_at(bottom_right);
        generate_alt_move_to(bottom_right + motion);

        Size const expected_size{
            std::min(max_width,  min_width  + delta*width_inc),
            std::min(max_height, min_height + delta*height_inc)};

        EXPECT_THAT(static_cast<float>(actual.width.as_int())/actual.height.as_int(), expected_aspect_ratio);
        EXPECT_THAT(actual, Eq(expected_size));
    };
}

TEST_F(SurfaceModifications, surface_spec_state_affects_surface_visibility)
{
    auto const new_state = mir_window_state_hidden;

    EXPECT_CALL(surface_observer, hidden_set_to(_, true));

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_state(spec, new_state);
        });
}

TEST_P(SurfaceSpecStateCase, set_state_affects_surface_visibility)
{
    auto const initial_state = GetParam().from;
    auto const new_state = GetParam().to;

    EXPECT_CALL(surface_observer, hidden_set_to(_, is_visible(initial_state)));
    EXPECT_CALL(surface_observer, hidden_set_to(_, is_visible(new_state)));

    apply_changes([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_state(spec, initial_state);
        });

    apply_changes([&](MirWindowSpec* spec)
       {
           mir_window_spec_set_state(spec, new_state);
       });
}

INSTANTIATE_TEST_SUITE_P(SurfaceModifications, SurfaceSpecStateCase,
    Values(
        StatePair{mir_window_state_hidden, mir_window_state_restored},
        StatePair{mir_window_state_hidden, mir_window_state_maximized},
        StatePair{mir_window_state_hidden, mir_window_state_vertmaximized},
        StatePair{mir_window_state_hidden, mir_window_state_horizmaximized},
        StatePair{mir_window_state_hidden, mir_window_state_fullscreen},
        StatePair{mir_window_state_minimized, mir_window_state_restored},
        StatePair{mir_window_state_minimized, mir_window_state_maximized},
        StatePair{mir_window_state_minimized, mir_window_state_vertmaximized},
        StatePair{mir_window_state_minimized, mir_window_state_horizmaximized},
        StatePair{mir_window_state_minimized, mir_window_state_fullscreen}
    ));

TEST_P(SurfaceStateCase, set_state_affects_surface_visibility)
{
    auto const initial_state = GetParam().from;
    auto const new_state = GetParam().to;

    mt::Signal received_initial_state;
    mt::Signal received_new_state;

    EXPECT_CALL(surface_observer, hidden_set_to(_, is_visible(initial_state)))
        .WillOnce(InvokeWithoutArgs([&]{ received_initial_state.raise(); }));
    EXPECT_CALL(surface_observer, hidden_set_to(_, is_visible(new_state)))
        .WillOnce(InvokeWithoutArgs([&]{ received_new_state.raise(); }));

    mir_window_set_state(window, initial_state);
    mir_window_set_state(window, new_state);

    received_initial_state.wait_for(400ms);
    received_new_state.wait_for(400ms);
}

INSTANTIATE_TEST_SUITE_P(SurfaceModifications, SurfaceStateCase,
    Values(
        StatePair{mir_window_state_hidden, mir_window_state_restored},
        StatePair{mir_window_state_hidden, mir_window_state_maximized},
        StatePair{mir_window_state_hidden, mir_window_state_vertmaximized},
        StatePair{mir_window_state_hidden, mir_window_state_horizmaximized},
        StatePair{mir_window_state_hidden, mir_window_state_fullscreen},
        StatePair{mir_window_state_minimized, mir_window_state_restored},
        StatePair{mir_window_state_minimized, mir_window_state_maximized},
        StatePair{mir_window_state_minimized, mir_window_state_vertmaximized},
        StatePair{mir_window_state_minimized, mir_window_state_horizmaximized},
        StatePair{mir_window_state_minimized, mir_window_state_fullscreen}
    ));
