/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "src/server/shell/decoration/basic_decoration.h"
#include "mir/scene/surface_observer.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/input/cursor_images.h"
#include "src/server/scene/basic_surface.h"
#include "src/server/report/null_report_factory.h"

#include "mir/test/fake_shared.h"
#include "mir/test/doubles/stub_shell.h"
#include "mir/test/doubles/mock_scene_session.h"
#include "mir/test/doubles/mock_buffer_stream.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/explicit_executor.h"
#include "mir/test/doubles/stub_cursor_image.h"
#include "mir/events/event_builders.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::scene;
namespace mi = mir::input;
namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mev = mir::events;
namespace msd = mir::shell::decoration;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;
using namespace std::chrono_literals;

namespace
{
geom::DeltaX const button_width{28};
geom::Size const default_window_size{240, 120};
geom::Point const local_point_on_titlebar{20, 7};
geom::Point const local_close_button_location{
    as_x(default_window_size.width) - button_width * 0.5,
    local_point_on_titlebar.y};
geom::Point const local_maximize_button_location{
    local_close_button_location - geom::Displacement{button_width, 0}};
geom::Point const local_minimize_button_location{
    local_maximize_button_location - geom::Displacement{button_width, 0}};

struct StubCursorImages
    : mi::CursorImages
{
    auto image(
        std::string const& /*cursor_name*/,
        geom::Size const& /*size*/) -> std::shared_ptr<mir::graphics::CursorImage> override
    {
        return std::make_shared<mtd::StubCursorImage>();
    }
};

struct MockSurface
    : ms::BasicSurface
{
    MockSurface(std::shared_ptr<ms::Session> const& session)
        : ms::BasicSurface{
              session,
              {},
              {{},{}},
              mir_pointer_unconfined,
              { { std::make_shared<testing::NiceMock<mtd::MockBufferStream>>(), {0, 0}, {} } },
              {},
              mir::report::null_scene_report()}
    {
    }

    MOCK_METHOD0(request_client_surface_close, void());
};

struct MockShell
    : mtd::StubShell
{
    void modify_surface(
        std::shared_ptr<ms::Session> const&,
        std::shared_ptr<ms::Surface> const& surface,
        msh::SurfaceSpecification const& original_spec) override
    {
        msh::SurfaceSpecification spec{original_spec};

        // consume all the properties we expect, and error at the end if anything is left over

        if (spec.width.is_set() && spec.height.is_set())
            surface->resize({spec.width.consume(), spec.height.consume()});

        if (spec.input_shape.is_set())
            surface->set_input_region(spec.input_shape.consume());

        if (spec.streams.is_set()) spec.streams.consume();
        if (spec.cursor_image.is_set()) spec.cursor_image.consume();
        if (spec.state.is_set()) spec.state.consume();

        EXPECT_TRUE(spec.is_empty()) << "State not cleared in MockShell::modify_surface()";

        did_modify_surface(surface, original_spec);
    }

    MOCK_METHOD2(did_modify_surface, void(
        std::shared_ptr<ms::Surface> const&,
        msh::SurfaceSpecification const&));

    MOCK_METHOD3(request_move, void(
        std::shared_ptr<ms::Session> const&,
        std::shared_ptr<ms::Surface> const&,
        uint64_t));

    MOCK_METHOD4(request_resize, void(
        std::shared_ptr<ms::Session> const&,
        std::shared_ptr<ms::Surface> const&,
        uint64_t,
        MirResizeEdge));

    MOCK_METHOD3(create_surface, std::shared_ptr<ms::Surface>(
        std::shared_ptr<ms::Session> const&,
        ms::SurfaceCreationParameters const&,
        std::shared_ptr<ms::SurfaceObserver> const&));

    MOCK_METHOD2(destroy_surface, void(
        std::shared_ptr<ms::Session> const&,
        std::shared_ptr<ms::Surface> const&));
};

struct DecorationBasicDecoration
    : Test
{
    void SetUp() override
    {
        ON_CALL(shell, create_surface(_, _, _))
            .WillByDefault(Invoke([this](
                    std::shared_ptr<ms::Session> const&,
                    ms::SurfaceCreationParameters const& params,
                    std::shared_ptr<ms::SurfaceObserver> const& observer) -> std::shared_ptr<ms::Surface>
                {
                    creation_params = params;
                    decoration_surface.resize(params.size);
                    decoration_surface.add_observer(observer);
                    return mt::fake_shared(decoration_surface);
                }));
        ON_CALL(*session, create_buffer_stream(_))
            .WillByDefault(Return(mt::fake_shared(buffer_stream)));
        window_surface.resize(default_window_size);
        basic_decoration = std::make_shared<msd::BasicDecoration>(
            mt::fake_shared(shell),
            mt::fake_shared(buffer_allocator),
            mt::fake_shared(executor),
            mt::fake_shared(cursor_images),
            mt::fake_shared(window_surface));
        executor.execute();
    }

    void decoration_event(mir::EventUPtr event)
    {
        decoration_surface.consume(event.get());
        executor.execute();
    }

    void TearDown() override
    {
        basic_decoration.reset();
    }

    NiceMock<MockShell> shell;
    mtd::StubBufferAllocator buffer_allocator;
    mtd::ExplicitExectutor executor;
    StubCursorImages cursor_images;
    std::shared_ptr<msd::BasicDecoration> basic_decoration;
    std::shared_ptr<NiceMock<mtd::MockSceneSession>> session{std::make_shared<NiceMock<mtd::MockSceneSession>>()};
    StrictMock<MockSurface> window_surface{session};
    StrictMock<MockSurface> decoration_surface{session};
    ms::SurfaceCreationParameters creation_params;
    NiceMock<mtd::MockBufferStream> buffer_stream;
};

struct ResizeParam
{
    geom::Point point;
    MirResizeEdge edge;
};

std::ostream& operator<<(std::ostream& ostream, ResizeParam const& param)
{
    std::string edge = "???";
    switch (param.edge)
    {
    case mir_resize_edge_none: edge = "none"; break;
    case mir_resize_edge_north: edge = "north"; break;
    case mir_resize_edge_south: edge = "south"; break;
    case mir_resize_edge_east: edge = "east"; break;
    case mir_resize_edge_west: edge = "west"; break;
    case mir_resize_edge_northeast: edge = "northeast"; break;
    case mir_resize_edge_northwest: edge = "northwest"; break;
    case mir_resize_edge_southeast: edge = "southeast"; break;
    case mir_resize_edge_southwest: edge = "southwest"; break;
    }
    return ostream << param.point << " " << edge;
}

struct ResizeBasicDecoration
    : DecorationBasicDecoration,
      WithParamInterface<ResizeParam>
{
};

/// Returns points near (but not at) the edges, corners and middle of a rect of the given size with top_left = origin
auto nine_points(geom::Size size) -> std::vector<std::vector<geom::Point>>
{
    std::vector<std::vector<geom::Point>> points{
        {{1, 1}, {1, 1}, {1, 1}},
        {{1, 1}, {1, 1}, {1, 1}},
        {{1, 1}, {1, 1}, {1, 1}}};
    for (auto x : {0, 1, 2}) points[1][x].y = as_y(size.height * 0.5);
    for (auto x : {0, 1, 2}) points[2][x].y = as_y(size.height) - geom::DeltaY{2};
    for (auto y : {0, 1, 2}) points[y][1].x = as_x(size.width * 0.5);
    for (auto y : {0, 1, 2}) points[y][2].x = as_x(size.width) - geom::DeltaX{2};
    return points;
}

auto pointer_event(
    MirPointerAction action,
    MirPointerButtons buttons_pressed,
    geom::Point position,
    std::chrono::nanoseconds timestamp = 0ns) -> mir::EventUPtr
{
    return mev::make_event(
        (MirInputDeviceId)1,
        timestamp + 1s,
        std::vector<uint8_t>{},
        mir_input_event_modifier_none,
        action,
        buttons_pressed,
        position.x.as_int(), position.y.as_int(),
        0, 0,
        0, 0);
}

auto touch_event(
    MirTouchId touch_id,
    MirTouchAction action,
    geom::Point position,
    std::chrono::nanoseconds timestamp = 0ns) -> mir::EventUPtr
{
    auto ev = mev::make_event(
        (MirInputDeviceId)1,
        timestamp + 1s,
        std::vector<uint8_t>{},
        mir_input_event_modifier_none);
    mev::add_touch(
        *ev,
        touch_id,
        action, mir_touch_tooltype_finger,
        position.x.as_int(), position.y.as_int(),
        1.0, 1.0, 1.0, 1.0);
    return ev;
}
}

TEST_F(DecorationBasicDecoration, can_be_created)
{
}

TEST_F(DecorationBasicDecoration, decoration_window_creation_params)
{
    ASSERT_THAT(creation_params.parent.lock(), Eq(mt::fake_shared(window_surface)));
    ASSERT_THAT(creation_params.size, Eq(window_surface.window_size()));
}

TEST_F(DecorationBasicDecoration, decoration_surface_removed_from_session_on_destroy)
{
    std::shared_ptr<ms::Surface> surface{mt::fake_shared(decoration_surface)};
    std::shared_ptr<ms::Session> session_{session};
    EXPECT_CALL(shell, destroy_surface(session_, surface))
        .Times(1);
    basic_decoration.reset();
    executor.execute();
}

TEST_F(DecorationBasicDecoration, redrawn_on_rename)
{
    EXPECT_CALL(buffer_stream, submit_buffer(_))
        .Times(AtLeast(1));
    window_surface.rename("new name");
    executor.execute();
    Mock::VerifyAndClearExpectations(&buffer_stream);
}

TEST_F(DecorationBasicDecoration, redrawn_on_focus_state_change)
{
    window_surface.configure(mir_window_attrib_focus, mir_window_focus_state_focused);
    executor.execute();
    Mock::VerifyAndClearExpectations(&buffer_stream);
    EXPECT_CALL(buffer_stream, submit_buffer(_))
        .Times(AtLeast(1));
    window_surface.configure(mir_window_attrib_focus, mir_window_focus_state_unfocused);
    executor.execute();
    Mock::VerifyAndClearExpectations(&buffer_stream);
}

TEST_F(DecorationBasicDecoration, decoration_resized_on_window_resize)
{
    geom::Size new_size{203, 305};
    std::shared_ptr<ms::Surface> decoration_surface_{mt::fake_shared(decoration_surface)};
    msh::SurfaceSpecification spec;
    EXPECT_CALL(shell, did_modify_surface(decoration_surface_, _))
        .Times(1)
        .WillOnce(SaveArg<1>(&spec));
    window_surface.resize(new_size);
    executor.execute();
    ASSERT_TRUE(spec.width.is_set());
    ASSERT_TRUE(spec.height.is_set());
    EXPECT_THAT(spec.width.value(), Eq(new_size.width));
    EXPECT_THAT(spec.height.value(), Eq(new_size.height));
}

TEST_F(DecorationBasicDecoration, makes_padding_for_borders)
{
    EXPECT_THAT(window_surface.content_size().width, Lt(window_surface.window_size().width));
    EXPECT_THAT(window_surface.content_size().height, Lt(window_surface.window_size().height));
}

TEST_F(DecorationBasicDecoration, input_areas_of_surfaces_line_up)
{
    for (geom::X x{0}; x < as_x(window_surface.window_size().width); x += geom::DeltaX{1})
    {
        for (geom::Y y{0}; y < as_y(window_surface.window_size().height); y += geom::DeltaY{1})
        {
            if (decoration_surface.input_area_contains({x, y}) && window_surface.input_area_contains({x, y}))
                FAIL() << geom::Point{x, y} << " is in the input area of both surfaces";

            if (!decoration_surface.input_area_contains({x, y}) && !window_surface.input_area_contains({x, y}))
                FAIL() << geom::Point{x, y} << " is not in the input area of either surface";
        }
    }
}

// Restored window tests (should have full borders)
// (maximize and then restore as this is testing updates, not initial setup)

TEST_F(DecorationBasicDecoration, padding_comes_back_when_restored)
{
    window_surface.configure(mir_window_attrib_state, mir_window_state_maximized);
    executor.execute();
    window_surface.configure(mir_window_attrib_state, mir_window_state_restored);
    executor.execute();
    EXPECT_THAT(window_surface.content_size().width, Lt(window_surface.window_size().width));
    EXPECT_THAT(window_surface.content_size().height, Lt(window_surface.window_size().height));
}

TEST_F(DecorationBasicDecoration, four_decoration_streams_when_restored)
{
    window_surface.configure(mir_window_attrib_state, mir_window_state_maximized);
    executor.execute();
    std::shared_ptr<ms::Surface> decoration_surface_{mt::fake_shared(decoration_surface)};
    msh::SurfaceSpecification spec;
    EXPECT_CALL(shell, did_modify_surface(decoration_surface_, _))
        .Times(1)
        .WillOnce(SaveArg<1>(&spec));
    window_surface.configure(mir_window_attrib_state, mir_window_state_restored);
    executor.execute();
    ASSERT_TRUE(spec.streams.is_set());
    EXPECT_THAT(spec.streams.value().size(), Eq(4)); // Titlebar and left, right and bottom borders
}

TEST_F(DecorationBasicDecoration, input_area_contains_borders_when_restored)
{
    window_surface.configure(mir_window_attrib_state, mir_window_state_maximized);
    executor.execute();
    window_surface.configure(mir_window_attrib_state, mir_window_state_restored);
    executor.execute();
    // points near the edges, corners and middle
    std::vector<std::string> const y_strs{"top", "center", "bottom"};
    std::vector<std::string> const x_strs{"left", "center", "right"};
    auto const points = nine_points(window_surface.window_size());
    std::vector<std::vector<bool>> const values{
        {   true,   true,   true},
        {   true,   false,  true},
        {   true,   true,   true}};
    for (unsigned x = 0; x < 3; x++)
        for (unsigned y = 0; y < 3; y++)
            ASSERT_THAT(decoration_surface.input_area_contains(points[y][x]), Eq(values[y][x]))
                << "   Point: " << y_strs[y] << "-" << x_strs[x] << " " << points[y][x];
}

// Maximized window tests (should only have titlebar)

TEST_F(DecorationBasicDecoration, only_has_titlebar_padding_when_maximized)
{
    window_surface.configure(mir_window_attrib_state, mir_window_state_maximized);
    executor.execute();
    EXPECT_THAT(window_surface.content_size().width, Eq(window_surface.window_size().width));
    EXPECT_THAT(window_surface.content_size().height, Lt(window_surface.window_size().height));
}

TEST_F(DecorationBasicDecoration, one_decoration_stream_when_maximized)
{
    std::shared_ptr<ms::Surface> decoration_surface_{mt::fake_shared(decoration_surface)};
    msh::SurfaceSpecification spec;
    EXPECT_CALL(shell, did_modify_surface(decoration_surface_, _))
        .Times(1)
        .WillOnce(SaveArg<1>(&spec));
    window_surface.configure(mir_window_attrib_state, mir_window_state_maximized);
    executor.execute();
    ASSERT_TRUE(spec.streams.is_set());
    EXPECT_THAT(spec.streams.value().size(), Eq(1)); // Titlebar only
}

TEST_F(DecorationBasicDecoration, input_area_contains_only_top_bar_when_maximized)
{
    window_surface.configure(mir_window_attrib_state, mir_window_state_maximized);
    executor.execute();
    // points near the edges, corners and middle
    std::vector<std::string> const y_strs{"top", "center", "bottom"};
    std::vector<std::string> const x_strs{"left", "center", "right"};
    auto const points = nine_points(window_surface.window_size());
    std::vector<std::vector<bool>> const values{
        {   true,   true,   true},
        {   false,  false,  false},
        {   false,  false,  false}};
    for (unsigned x = 0; x < 3; x++)
        for (unsigned y = 0; y < 3; y++)
            EXPECT_THAT(decoration_surface.input_area_contains(points[y][x]), Eq(values[y][x]))
                << "   Point: " << y_strs[y] << "-" << x_strs[x] << " " << points[y][x];
}

// Fullscreen window tests (should have no decorations)

TEST_F(DecorationBasicDecoration, no_padding_when_fullscreen)
{
    window_surface.configure(mir_window_attrib_state, mir_window_state_fullscreen);
    executor.execute();
    EXPECT_THAT(window_surface.content_size().width, Eq(window_surface.window_size().width));
    EXPECT_THAT(window_surface.content_size().height, Eq(window_surface.window_size().height));
}

TEST_F(DecorationBasicDecoration, zero_decoration_streams_when_fullscreen)
{
    std::shared_ptr<ms::Surface> decoration_surface_{mt::fake_shared(decoration_surface)};
    msh::SurfaceSpecification spec;
    EXPECT_CALL(shell, did_modify_surface(decoration_surface_, _))
        .Times(1)
        .WillOnce(SaveArg<1>(&spec));
    window_surface.configure(mir_window_attrib_state, mir_window_state_fullscreen);
    executor.execute();
    ASSERT_TRUE(spec.streams.is_set());
    EXPECT_THAT(spec.streams.value().size(), Eq(0)); // No decorations
}

TEST_F(DecorationBasicDecoration, input_area_contains_nothing_when_fullscreen)
{
    window_surface.configure(mir_window_attrib_state, mir_window_state_fullscreen);
    executor.execute();
    // points near the edges, corners and middle
    std::vector<std::string> const y_strs{"top", "center", "bottom"};
    std::vector<std::string> const x_strs{"left", "center", "right"};
    auto const points = nine_points(window_surface.window_size());
    std::vector<std::vector<bool>> const values{
        {   false,  false,  false},
        {   false,  false,  false},
        {   false,  false,  false}};
    for (unsigned x = 0; x < 3; x++)
        for (unsigned y = 0; y < 3; y++)
            EXPECT_THAT(decoration_surface.input_area_contains(points[y][x]), Eq(values[y][x]))
                << "   Point: " << y_strs[y] << "-" << x_strs[x] << " " << points[y][x];
}

TEST_F(DecorationBasicDecoration, moves_on_titlebar_pointer_drag)
{
    geom::Point const start_point{local_point_on_titlebar};
    geom::Point const move_to_point{start_point + geom::Displacement{5, 5}};
    ASSERT_TRUE(decoration_surface.input_area_contains(start_point)) << "Precondition failed";
    std::vector<mir::EventUPtr> events;
    std::shared_ptr<ms::Session> session_{session};
    std::shared_ptr<ms::Surface> window_surface_{mt::fake_shared(window_surface)};
    EXPECT_CALL(shell, request_move(session_, window_surface_, _))
        .Times(1);
    decoration_event(pointer_event(mir_pointer_action_enter, (MirPointerButtons)0, start_point));
    decoration_event(pointer_event(mir_pointer_action_button_down, mir_pointer_button_primary, start_point));
    decoration_event(pointer_event(mir_pointer_action_motion, mir_pointer_button_primary, move_to_point));
}

TEST_F(DecorationBasicDecoration, moves_on_titlebar_pointer_drag_starting_at_close_button)
{
    geom::Point const start_point{local_close_button_location};
    geom::Point const move_to_point{start_point + geom::Displacement{5, 5}};
    ASSERT_TRUE(decoration_surface.input_area_contains(start_point)) << "Precondition failed";
    std::shared_ptr<ms::Session> session_{session};
    std::shared_ptr<ms::Surface> window_surface_{mt::fake_shared(window_surface)};
    EXPECT_CALL(shell, request_move(session_, window_surface_, _))
        .Times(1);
    EXPECT_CALL(window_surface, request_client_surface_close())
        .Times(0);
    decoration_event(pointer_event(mir_pointer_action_enter, (MirPointerButtons)0, start_point));
    decoration_event(pointer_event(mir_pointer_action_button_down, mir_pointer_button_primary, start_point));
    decoration_event(pointer_event(mir_pointer_action_motion, mir_pointer_button_primary, move_to_point));
}

TEST_F(DecorationBasicDecoration, moves_on_titlebar_touch_drag)
{
    geom::Point const start_point{local_point_on_titlebar};
    geom::Point move_to_point{start_point + geom::Displacement{5, 5}};
    int32_t const touch_id{1};
    ASSERT_TRUE(decoration_surface.input_area_contains(start_point)) << "Precondition failed";
    std::shared_ptr<ms::Session> session_{session};
    std::shared_ptr<ms::Surface> window_surface_{mt::fake_shared(window_surface)};
    EXPECT_CALL(shell, request_move(session_, window_surface_, _))
        .Times(1);
    decoration_event(touch_event(touch_id, mir_touch_action_down, start_point));
    decoration_event(touch_event(touch_id, mir_touch_action_change, move_to_point));
}

TEST_F(DecorationBasicDecoration, closes_on_close_click)
{
    geom::Point const click_point{local_close_button_location};
    EXPECT_CALL(window_surface, request_client_surface_close())
        .Times(1);
    decoration_event(pointer_event(mir_pointer_action_enter, (MirPointerButtons)0, click_point));
    decoration_event(pointer_event(mir_pointer_action_button_down, mir_pointer_button_primary, click_point));
    decoration_event(pointer_event(mir_pointer_action_button_up, (MirPointerButtons)0, click_point));
}

TEST_F(DecorationBasicDecoration, closes_on_close_tap)
{
    geom::Point const tap_point{local_close_button_location};
    int32_t const touch_id{1};
    EXPECT_CALL(window_surface, request_client_surface_close())
        .Times(1);
    decoration_event(touch_event(touch_id, mir_touch_action_down, tap_point));
    decoration_event(touch_event(touch_id, mir_touch_action_up, tap_point));
}

TEST_F(DecorationBasicDecoration, maximizes_on_maximize_click)
{
    geom::Point const click_point{local_maximize_button_location};
    std::shared_ptr<ms::Surface> window_surface_{mt::fake_shared(window_surface)};
    msh::SurfaceSpecification spec;
    EXPECT_CALL(shell, did_modify_surface(window_surface_, _))
        .Times(1)
        .WillOnce(SaveArg<1>(&spec));
    decoration_event(pointer_event(mir_pointer_action_enter, (MirPointerButtons)0, click_point));
    decoration_event(pointer_event(mir_pointer_action_button_down, mir_pointer_button_primary, click_point));
    decoration_event(pointer_event(mir_pointer_action_button_up, (MirPointerButtons)0, click_point));
    ASSERT_TRUE(spec.state.is_set());
    EXPECT_THAT(spec.state.value(), Eq(mir_window_state_maximized));
}

TEST_F(DecorationBasicDecoration, restores_on_maximize_click_while_maximized)
{
    window_surface.configure(mir_window_attrib_state, mir_window_state_maximized);
    executor.execute();
    geom::Point const click_point{local_maximize_button_location};
    std::shared_ptr<ms::Surface> window_surface_{mt::fake_shared(window_surface)};
    msh::SurfaceSpecification spec;
    EXPECT_CALL(shell, did_modify_surface(window_surface_, _))
        .Times(1)
        .WillOnce(SaveArg<1>(&spec));
    decoration_event(pointer_event(mir_pointer_action_enter, (MirPointerButtons)0, click_point));
    decoration_event(pointer_event(mir_pointer_action_button_down, mir_pointer_button_primary, click_point));
    decoration_event(pointer_event(mir_pointer_action_button_up, (MirPointerButtons)0, click_point));
    ASSERT_TRUE(spec.state.is_set());
    EXPECT_THAT(spec.state.value(), Eq(mir_window_state_restored));
}

TEST_F(DecorationBasicDecoration, minimizes_on_minimize_click)
{
    geom::Point const click_point{local_minimize_button_location};
    std::vector<mir::EventUPtr> events;
    std::shared_ptr<ms::Surface> window_surface_{mt::fake_shared(window_surface)};
    msh::SurfaceSpecification spec;
    EXPECT_CALL(shell, did_modify_surface(window_surface_, _))
        .Times(1)
        .WillOnce(SaveArg<1>(&spec));
    decoration_event(pointer_event(mir_pointer_action_enter, (MirPointerButtons)0, click_point));
    decoration_event(pointer_event(mir_pointer_action_button_down, mir_pointer_button_primary, click_point));
    decoration_event(pointer_event(mir_pointer_action_button_up, (MirPointerButtons)0, click_point));
    ASSERT_TRUE(spec.state.is_set());
    EXPECT_THAT(spec.state.value(), Eq(mir_window_state_minimized));
}

TEST_P(ResizeBasicDecoration, resizes_on_pointer_drag)
{
    auto const param = GetParam();
    geom::Point const start_point{param.point};
    geom::Point const move_to_point{start_point + geom::Displacement{5, 5}};
    ASSERT_TRUE(decoration_surface.input_area_contains(start_point)) << "Precondition failed";
    std::shared_ptr<ms::Surface> window_surface_{mt::fake_shared(window_surface)};
    EXPECT_CALL(shell, request_resize(_, window_surface_, _, param.edge))
        .Times(1);
    decoration_event(pointer_event(mir_pointer_action_enter, (MirPointerButtons)0, start_point));
    decoration_event(pointer_event(mir_pointer_action_button_down, mir_pointer_button_primary, start_point));
    decoration_event(pointer_event(mir_pointer_action_motion, mir_pointer_button_primary, move_to_point));
}

TEST_P(ResizeBasicDecoration, resizes_on_touch_drag)
{
    auto const param = GetParam();
    geom::Point const start_point{param.point};
    geom::Point move_to_point{start_point + geom::Displacement{5, 5}};
    int32_t const touch_id{1};
    ASSERT_TRUE(decoration_surface.input_area_contains(start_point)) << "Precondition failed";
    std::shared_ptr<ms::Session> session_{session};
    std::shared_ptr<ms::Surface> window_surface_{mt::fake_shared(window_surface)};
    EXPECT_CALL(shell, request_resize(session_, window_surface_, _, param.edge))
        .Times(1);
    decoration_event(touch_event(touch_id, mir_touch_action_down, start_point));
    decoration_event(touch_event(touch_id, mir_touch_action_change, move_to_point));
}

TEST_P(ResizeBasicDecoration, sets_cursor)
{
    auto const param = GetParam();
    geom::Point const start_point{param.point};
    ASSERT_TRUE(decoration_surface.input_area_contains(start_point)) << "Precondition failed";
    std::shared_ptr<ms::Surface> decoration_surface_{mt::fake_shared(decoration_surface)};
    msh::SurfaceSpecification spec;
    EXPECT_CALL(shell, did_modify_surface(decoration_surface_, _))
        .Times(1)
        .WillOnce(SaveArg<1>(&spec));
    decoration_event(pointer_event(mir_pointer_action_enter, (MirPointerButtons)0, start_point));
    EXPECT_TRUE(spec.cursor_image.is_set());
}

INSTANTIATE_TEST_CASE_P(ResizeBasicDecorationInsideCorners, ResizeBasicDecoration, Values(
    ResizeParam{nine_points(default_window_size)[0][0], mir_resize_edge_northwest},
    ResizeParam{nine_points(default_window_size)[0][1], mir_resize_edge_north},
    ResizeParam{nine_points(default_window_size)[0][2], mir_resize_edge_northeast},
    ResizeParam{nine_points(default_window_size)[1][0], mir_resize_edge_west},
    // center resize doesn't make sense
    ResizeParam{nine_points(default_window_size)[1][2], mir_resize_edge_east},
    ResizeParam{nine_points(default_window_size)[2][0], mir_resize_edge_southwest},
    ResizeParam{nine_points(default_window_size)[2][1], mir_resize_edge_south},
    ResizeParam{nine_points(default_window_size)[2][2], mir_resize_edge_southeast}));
