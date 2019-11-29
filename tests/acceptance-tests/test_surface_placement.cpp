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

#include "mir_test_framework/temporary_environment_value.h"
#include "mir/test/doubles/wrap_shell_to_track_latest_surface.h"
#include "mir_test_framework/connected_client_headless_server.h"
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

namespace
{
struct SurfacePlacement : mtf::ConnectedClientHeadlessServer
{
    SurfacePlacement() { add_to_environment("MIR_SERVER_ENABLE_INPUT", "OFF"); }

    Rectangle const first_display {{  0, 0}, {640,  480}};
    Rectangle const second_display{{640, 0}, {640,  480}};

    // limit to cascade step (don't hard code title bar height)
    Displacement const max_cascade{20, 20};

    void SetUp() override
    {
        initial_display_layout({first_display, second_display});

        server.wrap_shell([this](std::shared_ptr<msh::Shell> const& wrapped)
        {
            auto const msc = std::make_shared<mtd::WrapShellToTrackLatestSurface>(wrapped);
            shell = msc;
            return msc;
        });

        mtf::ConnectedClientHeadlessServer::SetUp();

        init_pixel_format();
        make_active(first_display);
    }

    void TearDown() override
    {
        shell.reset();
        mtf::ConnectedClientHeadlessServer::TearDown();
    }

    std::shared_ptr<ms::Surface> latest_shell_surface() const
    {
        auto const result = shell->latest_surface.lock();
//      ASSERT_THAT(result, NotNull()); //<= doesn't compile!?
        EXPECT_THAT(result, NotNull());
        return result;
    }

    template<typename Specifier>
    MirWindow* create_normal_surface(int width, int height, Specifier const& specifier) const
    {
        auto const spec = mir_create_normal_window_spec(connection, width, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_window_spec_set_pixel_format(spec, pixel_format);
#pragma GCC diagnostic pop

        specifier(spec);

        auto const window = mir_create_window_sync(spec);
        mir_window_spec_release(spec);

        return window;
    }

    template<typename Specifier>
    MirWindow* create_surface(Specifier const& specifier) const
    {
        auto const spec = mir_create_window_spec(connection);

        specifier(spec);

        auto const window = mir_create_window_sync(spec);
        mir_window_spec_release(spec);

        return window;
    }

    MirWindow* create_normal_surface(int width, int height) const
    {
        return create_normal_surface(width, height, [](MirWindowSpec*){});
    }

    void make_active(Rectangle const& display)
    {
        auto const click_position = display.top_left + 0.5*as_displacement(display.size);

        MirInputDeviceId const device_id{7};

        auto const modifiers = mir_input_event_modifier_none;
        auto const depressed_buttons = mir_pointer_button_primary;

        auto const x_axis_value = click_position.x.as_int();
        auto const y_axis_value = click_position.y.as_int();
        auto const hscroll_value = 0.0;
        auto const vscroll_value = 0.0;
        auto const action = mir_pointer_action_button_down;
        auto const relative_x_value = 0.0;
        auto const relative_y_value = 0.0;


        auto const click_event = mev::make_event(device_id, std::chrono::nanoseconds(1), std::vector<uint8_t>{}, modifiers,
            action, depressed_buttons, x_axis_value, y_axis_value, hscroll_value,
            vscroll_value, relative_x_value, relative_y_value);

        server.the_shell()->handle(*click_event);
    }

    MirPixelFormat pixel_format{mir_pixel_format_invalid};

private:
    std::shared_ptr<mtd::WrapShellToTrackLatestSurface> shell;

    void init_pixel_format()
    {
        unsigned int valid_formats{0};
        MirPixelFormat pixel_formats[mir_pixel_formats] = { mir_pixel_format_invalid };
        mir_connection_get_available_surface_formats(connection, pixel_formats, mir_pixel_formats, &valid_formats);
        //select an 8 bit opaque format if we can
        for (auto i = 0u; i < valid_formats; i++)
        {
            if (pixel_formats[i] == mir_pixel_format_xbgr_8888 || pixel_formats[i] == mir_pixel_format_xrgb_8888)
            {
                pixel_format = pixel_formats[i];
                break;
            }
        }
    }
};
}

// Optically centered in a space means:
//
//  o horizontally centered, and positioned vertically such that the top margin
//    is half the bottom margin (vertical centering would look too low, and
//    would allow little room for cascading), unless this would leave any
//    part of the window off-screen or in shell space;
//
//  o otherwise, as close as possible to that position without any part of the
//    window being off-screen or in shell space, if possible;
//
//  o otherwise, as close as possible to that position while keeping all of its
//    title bar thickness in non-shell space. (For example, a dialog that is
//    taller than the screen should be positioned immediately below any menu
//    bar or shell panel at the top of the screen.)

TEST_F(SurfacePlacement, small_window_is_optically_centered_on_first_display)
{
    auto const width = 59;
    auto const height= 61;

    auto const geometric_centre = first_display.top_left +
        0.5*(as_displacement(first_display.size) - Displacement{width, height});

    auto const optically_centred = geometric_centre -
        DeltaY{(first_display.size.height.as_int()-height)/6};

    auto const window = create_normal_surface(width, height);
    auto const shell_surface = latest_shell_surface();
    ASSERT_THAT(shell_surface, NotNull());  // Compiles here

    EXPECT_THAT(shell_surface->top_left(), Eq(optically_centred));
    EXPECT_THAT(shell_surface->window_size(), Eq(Size{width, height}));

    mir_window_release_sync(window);
}

TEST_F(SurfacePlacement, which_second_display_acive_small_window_is_optically_centered_on_it)
{
    auto const width = 59;
    auto const height= 61;

    make_active(second_display);

    auto const geometric_centre = second_display.top_left +
        0.5*(as_displacement(second_display.size) - Displacement{width, height});

    auto const optically_centred = geometric_centre -
        DeltaY{(second_display.size.height.as_int()-height)/6};

    auto const window = create_normal_surface(width, height);
    auto const shell_surface = latest_shell_surface();
    ASSERT_THAT(shell_surface, NotNull());  // Compiles here

    EXPECT_THAT(shell_surface->top_left(), Eq(optically_centred));
    EXPECT_THAT(shell_surface->window_size(), Eq(Size{width, height}));

    mir_window_release_sync(window);
}

TEST_F(SurfacePlacement, medium_window_fitted_onto_first_display)
{
    auto const width = first_display.size.width.as_int();
    auto const height= first_display.size.height.as_int();

    auto const window = create_normal_surface(width, height);
    auto const shell_surface = latest_shell_surface();

    EXPECT_THAT(shell_surface->top_left(), Eq(first_display.top_left));
    EXPECT_THAT(shell_surface->window_size(), Eq(Size{width, height}));
    EXPECT_THAT(shell_surface->window_size(), Eq(first_display.size));

    mir_window_release_sync(window);
}

TEST_F(SurfacePlacement, big_window_keeps_top_on_first_display)
{
    auto const width = 2*first_display.size.width.as_int();
    auto const height= 2*first_display.size.height.as_int();

    auto const window = create_normal_surface(width, height);
    auto const shell_surface = latest_shell_surface();

    EXPECT_THAT(shell_surface->top_left(), Eq(Point{-width/4, 0}));
    EXPECT_THAT(shell_surface->window_size(), Eq(Size{width, height}));

    mir_window_release_sync(window);
}

TEST_F(SurfacePlacement, second_window_is_on_same_display_as_first)
{
    auto const width = 67;
    auto const height= 71;

    auto const surface1 = create_normal_surface(width, height);
    auto const shell_surface1 = latest_shell_surface();
    shell_surface1->move_to(second_display.top_left);

    auto const surface2 = create_normal_surface(width, height);
    auto const shell_surface2 = latest_shell_surface();

    EXPECT_TRUE(second_display.contains({shell_surface2->input_bounds()}));

    mir_window_release_sync(surface1);
    mir_window_release_sync(surface2);
}

// Cascaded, horizontally and/or vertically, relative to another window means:
//
//  o if vertically, positioned one standard title-bar height lower than the
//    other window, and if horizontally, positioned one standard title-bar
//    height to the right if in an LTR language, or one standard title-bar
//    height to the left in an RTL language — unless the resulting position
//    would leave any part of the window off-screen or in shell space;
//
//  o otherwise, positioned the same way, and shrunk the minimum amount
//    required to avoid extending off-screen or into shell space — unless this
//    would require making it smaller than its minimum width/height (for
//    example, if the window is not resizable at all);
//
//  o otherwise, placed at the top left (LTR) or top right (RTL) of the
//    display’s biggest non-shell space.
TEST_F(SurfacePlacement, second_window_is_cascaded_wrt_first)
{
    auto const width = 73;
    auto const height= 79;

    auto const surface1 = create_normal_surface(width, height);
    auto const shell_surface1 = latest_shell_surface();
    auto const surface2 = create_normal_surface(width, height);
    auto const shell_surface2 = latest_shell_surface();

    EXPECT_THAT(shell_surface2->top_left().x, Gt(shell_surface1->top_left().x));
    EXPECT_THAT(shell_surface2->top_left().y, Gt(shell_surface1->top_left().y));

    EXPECT_THAT(shell_surface2->top_left().x, Lt((shell_surface1->top_left()+max_cascade).x));
    EXPECT_THAT(shell_surface2->top_left().y, Lt((shell_surface1->top_left()+max_cascade).y));

    EXPECT_THAT(shell_surface2->window_size(), Eq(Size{width, height}));

    mir_window_release_sync(surface1);
    mir_window_release_sync(surface2);
}

// This is what is currently in the spec, but I think it's wrong
TEST_F(SurfacePlacement, DISABLED_medium_second_window_is_sized_when_cascaded_wrt_first)
{
    auto const width = first_display.size.width.as_int();
    auto const height= first_display.size.height.as_int();

    auto const surface1 = create_normal_surface(width, height);
    auto const shell_surface1 = latest_shell_surface();
    auto const surface2 = create_normal_surface(width, height);
    auto const shell_surface2 = latest_shell_surface();

    EXPECT_THAT(shell_surface2->top_left().x, Gt(shell_surface1->top_left().x));
    EXPECT_THAT(shell_surface2->top_left().y, Gt(shell_surface1->top_left().y));

    EXPECT_THAT(shell_surface2->top_left().x, Lt((shell_surface1->top_left()+max_cascade).x));
    EXPECT_THAT(shell_surface2->top_left().y, Lt((shell_surface1->top_left()+max_cascade).y));

    EXPECT_TRUE(first_display.contains({shell_surface2->input_bounds()}));
    EXPECT_THAT(shell_surface2->window_size(), Ne(Size{width, height}));

    mir_window_release_sync(surface1);
    mir_window_release_sync(surface2);
}

// This is not what is currently in the spec, but I think it's right
TEST_F(SurfacePlacement, medium_second_window_is_cascaded_wrt_first)
{
    auto const width = first_display.size.width.as_int();
    auto const height= first_display.size.height.as_int();

    auto const surface1 = create_normal_surface(width, height);
    auto const shell_surface1 = latest_shell_surface();
    auto const surface2 = create_normal_surface(width, height);
    auto const shell_surface2 = latest_shell_surface();

    EXPECT_THAT(shell_surface2->top_left().x, Gt(shell_surface1->top_left().x));
    EXPECT_THAT(shell_surface2->top_left().y, Gt(shell_surface1->top_left().y));

    EXPECT_THAT(shell_surface2->top_left().x, Lt((shell_surface1->top_left()+max_cascade).x));
    EXPECT_THAT(shell_surface2->top_left().y, Lt((shell_surface1->top_left()+max_cascade).y));

    EXPECT_TRUE(first_display.overlaps({shell_surface2->input_bounds()}));
    EXPECT_THAT(shell_surface2->window_size(), Eq(Size{width, height}));

    mir_window_release_sync(surface1);
    mir_window_release_sync(surface2);
}

TEST_F(SurfacePlacement, fullscreen_surface_is_sized_to_display)
{
    auto const window = create_normal_surface(10, 10, [](MirWindowSpec* spec)
        {
            mir_window_spec_set_state(spec, mir_window_state_fullscreen);
        });

    auto const shell_surface = latest_shell_surface();

    EXPECT_THAT(shell_surface->top_left(), Eq(first_display.top_left));
    EXPECT_THAT(shell_surface->window_size(), Eq(first_display.size));

    mir_window_release_sync(window);
}

TEST_F(SurfacePlacement, maximized_surface_is_sized_to_display)
{
    auto const window = create_normal_surface(10, 10, [](MirWindowSpec* spec)
        {
            mir_window_spec_set_state(spec, mir_window_state_maximized);
        });

    auto const shell_surface = latest_shell_surface();

    EXPECT_THAT(shell_surface->top_left().x, Eq(first_display.top_left.x));

    // Allow for a titlebar...
    EXPECT_THAT(shell_surface->top_left().y, Lt((first_display.top_left+max_cascade).y));

    auto const expected_size = as_size(as_displacement(first_display.size)
        + (first_display.top_left - shell_surface->top_left()));

    EXPECT_THAT(shell_surface->window_size(), Eq(expected_size));

    mir_window_release_sync(window);
}

TEST_F(SurfacePlacement, horizmaximized_surface_is_sized_to_display)
{
    auto const window = create_normal_surface(10, 10, [](MirWindowSpec* spec)
        {
            mir_window_spec_set_state(spec, mir_window_state_horizmaximized);
        });

    auto const shell_surface = latest_shell_surface();

    EXPECT_THAT(shell_surface->top_left().x, Eq(first_display.top_left.x));

    // Allow for a titlebar...
    EXPECT_THAT(shell_surface->top_left().y, Gt((first_display.top_left+max_cascade).y));
    EXPECT_THAT(shell_surface->window_size().height, Eq(Height{10}));
    EXPECT_THAT(shell_surface->window_size().width, Eq(first_display.size.width));

    mir_window_release_sync(window);
}

TEST_F(SurfacePlacement, vertmaximized_surface_is_sized_to_display)
{
    auto const window = create_normal_surface(10, 10, [](MirWindowSpec* spec)
        {
            mir_window_spec_set_state(spec, mir_window_state_vertmaximized);
        });

    auto const shell_surface = latest_shell_surface();

    EXPECT_THAT(shell_surface->top_left().x, Gt(first_display.top_left.x));

    // Allow for a titlebar...
    EXPECT_THAT(shell_surface->top_left().y, Lt((first_display.top_left+max_cascade).y));

    Size const expected_size{10,
        first_display.size.height + (first_display.top_left.y-shell_surface->top_left().y)};

    EXPECT_THAT(shell_surface->window_size(), Eq(expected_size));

    mir_window_release_sync(window);
}

TEST_F(SurfacePlacement, fullscreen_on_output_1_surface_is_sized_to_first_display)
{
    auto const window = create_normal_surface(10, 10, [](MirWindowSpec* spec)
        {
            mir_window_spec_set_fullscreen_on_output(spec, 1);
        });

    auto const shell_surface = latest_shell_surface();

    EXPECT_THAT(shell_surface->top_left(), Eq(first_display.top_left));
    EXPECT_THAT(shell_surface->window_size(), Eq(first_display.size));

    mir_window_release_sync(window);
}

TEST_F(SurfacePlacement, fullscreen_on_output_2_surface_is_sized_to_second_display)
{
    auto const window = create_normal_surface(10, 10, [](MirWindowSpec* spec)
        {
            mir_window_spec_set_fullscreen_on_output(spec, 2);
        });

    auto const shell_surface = latest_shell_surface();

    EXPECT_THAT(shell_surface->top_left(), Eq(second_display.top_left));
    EXPECT_THAT(shell_surface->window_size(), Eq(second_display.size));

    mir_window_release_sync(window);
}

struct UnparentedSurface : SurfacePlacement, ::testing::WithParamInterface<MirWindowType> {};

TEST_P(UnparentedSurface, small_window_is_optically_centered_on_first_display)
{
    auto const width = 83;
    auto const height= 89;

    auto const geometric_centre = first_display.top_left +
                                  0.5*(as_displacement(first_display.size) - Displacement{width, height});

    auto const optically_centred = geometric_centre -
                                   DeltaY{(first_display.size.height.as_int()-height)/6};

    auto const window = create_surface([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, GetParam());
            mir_window_spec_set_width(spec, width);
            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            mir_window_spec_set_pixel_format(spec, pixel_format);
            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
        });

    auto const shell_surface = latest_shell_surface();
    ASSERT_THAT(shell_surface, NotNull());  // Compiles here

    EXPECT_THAT(shell_surface->top_left(), Eq(optically_centred));
    EXPECT_THAT(shell_surface->window_size(), Eq(Size{width, height}));

    mir_window_release_sync(window);
}

INSTANTIATE_TEST_CASE_P(SurfacePlacement, UnparentedSurface,
    ::testing::Values(
        mir_window_type_normal,
        mir_window_type_utility,
        mir_window_type_dialog,
        mir_window_type_freestyle));

// Parented dialog or parented freestyle window
//
// For convenience, these types are referred to here as “parented dialogs”.
//
//  o If a newly-opened parented dialog is the same as a previous dialog with
//    the same parent window, and it has user-customized position, then:
//      o …
//
//  o Otherwise, if the dialog is not the same as any previous dialog for the
//    same parent window, and/or it does not have user-customized position:
//      o It should be optically centered relative to its parent, unless this
//        would overlap or cover the title bar of the parent.
//      o Otherwise, it should be cascaded vertically (but not horizontally)
//        relative to its parent, unless, this would cause at least part of
//        it to extend into shell space.
//
// o Otherwise (resorting to the original plan) it should be optically centered
//    relative to its parent
// TODO tests for this

struct ParentedSurface : SurfacePlacement, ::testing::WithParamInterface<MirWindowType> {};

TEST_P(ParentedSurface, small_window_is_optically_centered_on_parent)
{
    auto const parent = create_normal_surface(256, 256);
    auto const shell_parent = latest_shell_surface();

    auto const width = 97;
    auto const height= 101;

    auto const geometric_centre = shell_parent->top_left() +
                                  0.5*(as_displacement(shell_parent->window_size()) - Displacement{width, height});

    auto const optically_centred = geometric_centre -
                                   DeltaY{(shell_parent->window_size().height.as_int()-height)/6};

    auto const window = create_surface([&](MirWindowSpec* spec)
        {
            mir_window_spec_set_type(spec, GetParam());
            mir_window_spec_set_width(spec, width);
            mir_window_spec_set_height(spec, height);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            mir_window_spec_set_pixel_format(spec, pixel_format);
            mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
            mir_window_spec_set_parent(spec, parent);
        });

    auto const shell_surface = latest_shell_surface();
    ASSERT_THAT(shell_surface, NotNull());  // Compiles here

    EXPECT_THAT(shell_surface->top_left(), Eq(optically_centred));
    EXPECT_THAT(shell_surface->window_size(), Eq(Size{width, height}));

    mir_window_release_sync(window);
    mir_window_release_sync(parent);
}

INSTANTIATE_TEST_CASE_P(SurfacePlacement, ParentedSurface,
    ::testing::Values(
        mir_window_type_dialog,
        mir_window_type_satellite,
        mir_window_type_menu,
        mir_window_type_gloss,
        mir_window_type_tip,
        mir_window_type_freestyle));
