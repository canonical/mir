/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/debug/surface.h"

#include "mir/test/doubles/mock_window_manager.h"

#include "mir/scene/session.h"
#include "mir/geometry/rectangle.h"

#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/validity_matchers.h"
#include "mir/test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <condition_variable>
#include <mutex>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mg = mir::geometry;

namespace
{
struct ClientSurfaces : mtf::ConnectedClientHeadlessServer
{
    static const int max_surface_count = 5;
    MirWindow* window[max_surface_count] = {nullptr};

    void SetUp() override
    {
        server.override_the_window_manager_builder([this](msh::FocusController*)
        {
            return mt::fake_shared(window_manager);
        });
        ConnectedClientHeadlessServer::SetUp();
    }

    testing::NiceMock<mtd::MockWindowManager> window_manager;
};
}

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
    EXPECT_EQ(mir_buffer_usage_hardware, response_params.buffer_usage);

    mir_window_get_parameters(window[1], &response_params);
    EXPECT_EQ(1600, response_params.width);
    EXPECT_EQ(1200, response_params.height);
    EXPECT_EQ(mir_pixel_format_abgr_8888, response_params.pixel_format);
    EXPECT_EQ(mir_buffer_usage_hardware, response_params.buffer_usage);

    mir_window_release_sync(window[1]);
    mir_window_release_sync(window[0]);
}

TEST_F(ClientSurfaces, have_distinct_ids)
{
    auto surface_1 = mtf::make_any_surface(connection);
    auto surface_2 = mtf::make_any_surface(connection);
    
    EXPECT_NE(mir_debug_window_id(surface_1),
        mir_debug_window_id(surface_2));

    mir_window_release_sync(surface_1);
    mir_window_release_sync(surface_2);
}

TEST_F(ClientSurfaces, creates_need_not_be_serialized)
{
    for (int i = 0; i != max_surface_count; ++i)
    {
        auto spec = mir_create_normal_window_spec(connection, 1, 1);
        mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
        window[i] = mir_create_window_sync(spec);
        mir_window_spec_release(spec);
    }

    for (int i = 0; i != max_surface_count; ++i)
    {
        for (int j = 0; j != max_surface_count; ++j)
        {
            if (i == j)
                EXPECT_EQ(
                    mir_debug_window_id(window[i]),
                    mir_debug_window_id(window[j]));
            else
                EXPECT_NE(
                    mir_debug_window_id(window[i]),
                    mir_debug_window_id(window[j]));
        }
    }

    for (int i = 0; i != max_surface_count; ++i)
        mir_window_release_sync(window[i]);
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

INSTANTIATE_TEST_CASE_P(ClientSurfaces,
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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(ClientSurfaces, input_methods_get_corret_parent_coordinates)
{
    using namespace testing;

    mg::Rectangle const server_rect{mg::Point{100, 100}, mg::Size{10, 10}};
    MirRectangle client_rect{ 100, 100, 10, 10 };

    auto const edge_attachment = mir_edge_attachment_vertical;

    std::shared_ptr<ms::Surface> parent_surface;
    InSequence seq;
    EXPECT_CALL(window_manager, add_surface(_,_,_)).WillOnce(Invoke([&parent_surface](auto session, auto params, auto builder)
    {
        auto id = builder(session, params);
        parent_surface = session->surface(id);
        return id;
    }));
    EXPECT_CALL(window_manager, add_surface(_,_,_)).WillOnce(Invoke([&parent_surface, server_rect, edge_attachment](auto session, auto params, auto builder)
    {
        EXPECT_THAT(params.parent.lock(), Eq(parent_surface));
        EXPECT_TRUE(params.aux_rect.is_set());
        EXPECT_THAT(params.aux_rect.value(), Eq(server_rect));
        EXPECT_TRUE(params.edge_attachment.is_set());
        EXPECT_THAT(params.edge_attachment.value(), Eq(edge_attachment));

        return builder(session, params);
    }));

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
