/*
 * Copyright © 2012-2017 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_toolkit/mir_client_library.h"

#include "miral/application_info.h"

#include "mir/scene/session.h"
#include "mir/geometry/rectangle.h"

#include "mir_test_framework/canonical_window_manager_policy.h"
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/any_surface.h"

#include "mir/test/validity_matchers.h"
#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>

namespace mtf = mir_test_framework;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mg = mir::geometry;
using namespace testing;

namespace
{
struct MockWindowManagementPolicy : mtf::CanonicalWindowManagerPolicy
{
    MockWindowManagementPolicy(
        miral::WindowManagerTools const& tools,
        MockWindowManagementPolicy*& self) :
        mtf::CanonicalWindowManagerPolicy(tools)
    {
        self = this;
        ON_CALL(*this, place_new_window(_, _)).WillByDefault(ReturnArg<1>());
    }

    MOCK_METHOD2(place_new_window,
        miral::WindowSpecification(miral::ApplicationInfo const& app_info, miral::WindowSpecification const& request_parameters));

    MOCK_METHOD1(advise_new_window, void (miral::WindowInfo const& window_info));

    void handle_request_drag_and_drop(miral::WindowInfo& /*window_info*/) {}
    void handle_request_move(miral::WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/) {}
    void handle_request_resize(miral::WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/, MirResizeEdge /*edge*/) {}
    mg::Rectangle confirm_placement_on_display(const miral::WindowInfo&, MirWindowState, mir::geometry::Rectangle const& new_placement)
    {
        return new_placement;
    }
};

struct ClientSurfaces : mtf::ConnectedClientHeadlessServer
{
    static const int max_surface_count = 5;
    MirWindow* window[max_surface_count] = {nullptr};

    void SetUp() override
    {
        override_window_management_policy<MockWindowManagementPolicy>(mock_wm_policy);
        mtf::ConnectedClientHeadlessServer::SetUp();

        ASSERT_THAT(mock_wm_policy, NotNull());
    }

    MockWindowManagementPolicy* mock_wm_policy = 0;
};
}
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(ClientSurfaces, are_created_with_correct_size)
{
    int width_1 = 640, height_1 = 480, width_2 = 1600, height_2 = 1200;
    
    auto spec = mir_create_normal_window_spec(connection, width_1, height_1);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
    window[0] = mir_create_window_sync(spec);

    mir_window_spec_set_width(spec, width_2);
    mir_window_spec_set_height(spec, height_2);

    window[1] = mir_create_window_sync(spec);
    
    mir_window_spec_release(spec);

    MirWindowParameters response_params;
    mir_window_get_parameters(window[0], &response_params);
    EXPECT_EQ(640, response_params.width);
    EXPECT_EQ(480, response_params.height);
    EXPECT_EQ(mir_pixel_format_abgr_8888, response_params.pixel_format);
    EXPECT_EQ(mir_buffer_usage_software, response_params.buffer_usage);

    mir_window_get_parameters(window[1], &response_params);
    EXPECT_EQ(1600, response_params.width);
    EXPECT_EQ(1200, response_params.height);
    EXPECT_EQ(mir_pixel_format_abgr_8888, response_params.pixel_format);
    EXPECT_EQ(mir_buffer_usage_software, response_params.buffer_usage);

    mir_window_release_sync(window[1]);
    mir_window_release_sync(window[0]);
}

struct WithOrientation : ClientSurfaces, ::testing::WithParamInterface<MirOrientationMode> {};

TEST_P(WithOrientation, have_requested_preferred_orientation)
{
    auto spec = mir_create_normal_window_spec(connection, 1, 1);
    ASSERT_THAT(spec, NotNull());
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    MirOrientationMode mode{GetParam()};
    mir_window_spec_set_preferred_orientation(spec, mode);

    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    ASSERT_THAT(window, IsValid());
    EXPECT_EQ(mir_window_get_preferred_orientation(window), mode);

    mir_window_release_sync(window);
}

INSTANTIATE_TEST_SUITE_P(ClientSurfaces,
    WithOrientation, ::testing::Values(
        mir_orientation_mode_portrait, mir_orientation_mode_landscape,
        mir_orientation_mode_portrait_inverted, mir_orientation_mode_landscape_inverted,
        mir_orientation_mode_portrait_any, mir_orientation_mode_landscape_any,
        mir_orientation_mode_any));

TEST_F(ClientSurfaces, can_be_menus)
{
    auto parent = mtf::make_any_surface(connection);
    MirRectangle attachment_rect{100, 200, 100, 100};

    auto spec = mir_create_menu_window_spec(connection, 640, 480,
        parent, &attachment_rect, mir_edge_attachment_vertical);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
    ASSERT_THAT(spec, NotNull());

    auto menu = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    ASSERT_THAT(menu, IsValid());
    EXPECT_EQ(mir_window_get_type(menu), mir_window_type_menu);

    mir_window_release_sync(parent);
    mir_window_release_sync(menu);
}

TEST_F(ClientSurfaces, can_be_tips)
{
    auto parent = mtf::make_any_surface(connection);
    MirRectangle rect{100, 200, 100, 100};

    auto spec = mir_create_tip_window_spec(connection, 640, 480,
        parent, &rect, mir_edge_attachment_any);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
    ASSERT_THAT(spec, NotNull());

    auto tooltip = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    ASSERT_THAT(tooltip, IsValid());
    EXPECT_EQ(mir_window_get_type(tooltip), mir_window_type_tip);

    mir_window_release_sync(parent);
    mir_window_release_sync(tooltip);
}

TEST_F(ClientSurfaces, can_be_dialogs)
{
    auto spec = mir_create_dialog_window_spec(connection, 640, 480);
    ASSERT_THAT(spec, NotNull());
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    auto dialog = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    ASSERT_THAT(dialog, IsValid());
    EXPECT_EQ(mir_window_get_type(dialog), mir_window_type_dialog);

    mir_window_release_sync(dialog);
}

TEST_F(ClientSurfaces, can_be_modal_dialogs)
{
    auto parent = mtf::make_any_surface(connection);
    auto spec = mir_create_modal_dialog_window_spec(connection, 640, 480, parent);
    ASSERT_THAT(spec, NotNull());
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    auto dialog = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    ASSERT_THAT(dialog, IsValid());
    EXPECT_EQ(mir_window_get_type(dialog), mir_window_type_dialog);

    mir_window_release_sync(parent);
    mir_window_release_sync(dialog);
}

TEST_F(ClientSurfaces, can_be_input_methods)
{
    auto spec = mir_create_input_method_window_spec(connection, 640, 480);
    ASSERT_THAT(spec, NotNull());
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    auto im = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_EQ(mir_window_get_type(im), mir_window_type_inputmethod);

    mir_window_release_sync(im);
}

TEST_F(ClientSurfaces, can_be_gloss)
{
    auto spec = mir_create_gloss_window_spec(connection, 640, 480);
    ASSERT_THAT(spec, NotNull());
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);
    EXPECT_EQ(mir_window_get_type(window), mir_window_type_gloss);

    mir_window_release_sync(window);
}

TEST_F(ClientSurfaces, can_be_satellite)
{
    auto parent = mtf::make_any_surface(connection);
    auto spec = mir_create_satellite_window_spec(connection, 640, 480, parent);
    ASSERT_THAT(spec, NotNull());
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_EQ(mir_window_get_type(window), mir_window_type_satellite);

    mir_window_release_sync(window);
    mir_window_release_sync(parent);
}

TEST_F(ClientSurfaces, can_be_utility)
{
    auto spec = mir_create_utility_window_spec(connection, 640, 480);
    ASSERT_THAT(spec, NotNull());
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_EQ(mir_window_get_type(window), mir_window_type_utility);

    mir_window_release_sync(window);
}

TEST_F(ClientSurfaces, can_be_freestyle)
{
    auto spec = mir_create_freestyle_window_spec(connection, 640, 480);
    ASSERT_THAT(spec, NotNull());
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);

    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_EQ(mir_window_get_type(window), mir_window_type_freestyle);

    mir_window_release_sync(window);
}

TEST_F(ClientSurfaces, can_be_renamed)
{
    auto spec = mir_create_normal_window_spec(connection, 123, 456);
    ASSERT_THAT(spec, NotNull());
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    /*
     * Generally no windowing system ever censors window names. They are
     * freeform strings set by the app.
     *
     * We do lack a getter to be able to read the name back and verify
     * these, but really don't care -- such a function is not required
     * right now. Although might be in future to support some toolkits.
     *
     * At least verify the rename completes without blocking...
     */
    spec = mir_create_window_spec(connection);
    ASSERT_THAT(spec, NotNull());
    mir_window_spec_set_name(spec, "New Name");
    mir_window_apply_spec(window, spec);

    mir_window_spec_set_name(spec, "");
    mir_window_apply_spec(window, spec);

    mir_window_spec_set_name(spec, "Alice");
    mir_window_apply_spec(window, spec);
    mir_window_spec_release(spec);

    mir_window_release_sync(window);
}


TEST_F(ClientSurfaces, input_methods_get_corret_parent_coordinates)
{
    using namespace testing;

    mg::Rectangle const server_rect{mg::Point{100, 100}, mg::Size{10, 10}};
    MirRectangle client_rect{ 100, 100, 10, 10 };

    auto const edge_attachment = mir_edge_attachment_vertical;

    std::shared_ptr<ms::Surface> parent_surface;
    InSequence seq;
    EXPECT_CALL(*mock_wm_policy, place_new_window(_, _)).Times(1);
    EXPECT_CALL(*mock_wm_policy, advise_new_window(_)).WillOnce(Invoke([&parent_surface](auto const& info)
        {
            parent_surface = info.window();
        }));

    EXPECT_CALL(*mock_wm_policy, place_new_window(_, _)).WillOnce(Invoke([&](auto const&, auto const& params)
        {
            EXPECT_TRUE(params.parent().is_set());
            EXPECT_THAT(params.parent().value().lock(), Eq(parent_surface));
            EXPECT_TRUE(params.aux_rect().is_set());
            EXPECT_THAT(params.aux_rect().value(), Eq(server_rect));
            EXPECT_TRUE(params.window_placement_gravity().is_set());
            EXPECT_THAT(params.window_placement_gravity().value(), Eq(mir_placement_gravity_northwest));
            EXPECT_TRUE(params.aux_rect_placement_gravity().is_set());
            EXPECT_THAT(params.aux_rect_placement_gravity().value(), Eq(mir_placement_gravity_northeast));
            EXPECT_TRUE(params.placement_hints().is_set());
            EXPECT_THAT(params.placement_hints().value(), Eq(mir_placement_hints_flip_x));
            return params;
        }));
    EXPECT_CALL(*mock_wm_policy, advise_new_window(_)).Times(1);

    auto window = mtf::make_any_surface(connection);

    auto parent_id = mir_window_request_persistent_id_sync(window);

    auto im_connection = mir_connect_sync(new_connection().c_str(), "Mock IM connection");
    ASSERT_THAT(im_connection, IsValid());

    auto spec = mir_create_input_method_window_spec(im_connection, 100, 20);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
    ASSERT_THAT(spec, NotNull());

    mir_window_spec_attach_to_foreign_parent(spec, parent_id, &client_rect, edge_attachment);

    mir_persistent_id_release(parent_id);

    auto im = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    mir_window_release_sync(im);
    mir_window_release_sync(window);

    mir_connection_release(im_connection);
}
#pragma GCC diagnostic pop
