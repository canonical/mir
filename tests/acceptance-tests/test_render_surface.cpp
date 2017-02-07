/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/rs/mir_render_surface.h"
#include "mir_toolkit/mir_presentation_chain.h"

#include "mir/geometry/size.h"
#include "mir_test_framework/headless_in_process_server.h"
#include "mir/test/validity_matchers.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace geom = mir::geometry;

namespace
{
struct RenderSurfaceTest : mtf::HeadlessInProcessServer
{
    geom::Size const logical_size{640, 480};
};
}

using namespace testing;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
TEST_F(RenderSurfaceTest, creates_and_releases_render_surfaces)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());

    ASSERT_THAT(rs, NotNull());
    EXPECT_TRUE(mir_render_surface_is_valid(rs));
    EXPECT_THAT(mir_render_surface_get_error_message(rs), StrEq(""));

    mir_render_surface_release(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, can_hand_out_buffer_stream)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());

    auto determine_physical_size = [](MirRenderSurface* rs) -> geom::Size
    {
        int width = -1;
        int height = -1;
        mir_render_surface_get_size(rs, &width, &height);
        return {width, height};
    };
    auto physical_size = determine_physical_size(rs);
    auto bs = mir_render_surface_get_buffer_stream(
        rs,
        physical_size.width.as_int(), physical_size.height.as_int(),
        mir_pixel_format_abgr_8888);

    ASSERT_THAT(bs, NotNull());
    EXPECT_TRUE(mir_buffer_stream_is_valid(bs));
    EXPECT_THAT(mir_buffer_stream_get_error_message(bs), StrEq(""));

    mir_render_surface_release(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, hands_out_buffer_stream_only_once)
{
    auto physical_size = logical_size;
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());

    auto bs = mir_render_surface_get_buffer_stream(
        rs,
        physical_size.width.as_int(), physical_size.height.as_int(),
        mir_pixel_format_abgr_8888);

    ASSERT_THAT(bs, NotNull());
    EXPECT_TRUE(mir_buffer_stream_is_valid(bs));
    EXPECT_THAT(mir_buffer_stream_get_error_message(bs), StrEq(""));

    auto bs2 = mir_render_surface_get_buffer_stream(
        rs,
        physical_size.width.as_int(), physical_size.height.as_int(),
        mir_pixel_format_abgr_8888);

    EXPECT_THAT(bs2, Eq(nullptr));

    mir_render_surface_release(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, dont_have_to_release_buffer_stream)
{
    auto physical_size = logical_size;
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());
    auto bs = mir_render_surface_get_buffer_stream(
        rs,
        physical_size.width.as_int(), physical_size.height.as_int(),
        mir_pixel_format_abgr_8888);

    ASSERT_THAT(bs, NotNull());
    EXPECT_TRUE(mir_buffer_stream_is_valid(bs));
    EXPECT_THAT(mir_buffer_stream_get_error_message(bs), StrEq(""));

    mir_render_surface_release(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, render_surfaces_without_content_can_be_added_to_spec)
{
    auto physical_size = logical_size;
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());
    auto spec = mir_create_normal_window_spec(
        connection,
        physical_size.width.as_int(), physical_size.height.as_int());
    mir_window_spec_add_render_surface(
        spec, rs, logical_size.width.as_int(), logical_size.height.as_int(), 0, 0);
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

    EXPECT_THAT(window, IsValid());
    EXPECT_THAT(mir_window_get_buffer_stream(window), Eq(nullptr));

    mir_render_surface_release(rs);
    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, stream_can_be_constructed_after_surface_creation)
{
    int const width{800}, height{600};
    MirPixelFormat const format{mir_pixel_format_abgr_8888};

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());
    auto spec = mir_create_normal_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);
    mir_window_spec_add_render_surface(spec, rs, width, height, 0, 0);
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);
    auto bs = mir_render_surface_get_buffer_stream(rs, 640, 480, format);

    EXPECT_THAT(window, IsValid());
    EXPECT_THAT(mir_window_get_buffer_stream(window), Eq(nullptr));
    EXPECT_TRUE(mir_buffer_stream_is_valid(bs));

    mir_render_surface_release(rs);
    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, can_hand_out_presentation_chain)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());

    auto pc = mir_render_surface_get_presentation_chain(rs);

    ASSERT_THAT(pc, NotNull());
    EXPECT_TRUE(mir_presentation_chain_is_valid(pc));

    mir_render_surface_release(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, hands_out_presentation_chain_only_once)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());

    auto pc = mir_render_surface_get_presentation_chain(rs);

    ASSERT_THAT(pc, NotNull());
    EXPECT_TRUE(mir_presentation_chain_is_valid(pc));

    auto pc2 = mir_render_surface_get_presentation_chain(rs);

    EXPECT_THAT(pc2, Eq(nullptr));

    mir_render_surface_release(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, chain_can_be_constructed_after_surface_creation)
{
    int const width{800}, height{600};
    MirPixelFormat const format{mir_pixel_format_abgr_8888};

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());
    auto spec = mir_create_normal_window_spec(connection, width, height);
    mir_window_spec_set_pixel_format(spec, format);
    mir_window_spec_add_render_surface(spec, rs, width, height, 0, 0);
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);
    auto pc = mir_render_surface_get_presentation_chain(rs);

    EXPECT_THAT(window, IsValid());
    EXPECT_THAT(mir_window_get_buffer_stream(window), Eq(nullptr));
    EXPECT_TRUE(mir_presentation_chain_is_valid(pc));

    mir_render_surface_release(rs);
    mir_window_release_sync(window);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, dont_have_to_release_presentation_chain)
{
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());
    auto pc = mir_render_surface_get_presentation_chain(rs);

    ASSERT_THAT(pc, NotNull());
    EXPECT_TRUE(mir_presentation_chain_is_valid(pc));

    mir_render_surface_release(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, excepts_on_stream_request_if_chain_handed_out)
{
    auto physical_size = logical_size;
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());
    auto pc = mir_render_surface_get_presentation_chain(rs);

    ASSERT_THAT(pc, NotNull());
    EXPECT_TRUE(mir_presentation_chain_is_valid(pc));

    auto bs = mir_render_surface_get_buffer_stream(
        rs,
        physical_size.width.as_int(), physical_size.height.as_int(),
        mir_pixel_format_abgr_8888);

    EXPECT_THAT(bs, Eq(nullptr));

    mir_render_surface_release(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, excepts_on_chain_request_if_stream_handed_out)
{
    auto physical_size = logical_size;
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());
    auto bs = mir_render_surface_get_buffer_stream(
        rs,
        physical_size.width.as_int(), physical_size.height.as_int(),
        mir_pixel_format_abgr_8888);

    ASSERT_THAT(bs, NotNull());
    EXPECT_TRUE(mir_buffer_stream_is_valid(bs));

    auto pc = mir_render_surface_get_presentation_chain(rs);

    EXPECT_THAT(pc, Eq(nullptr));

    mir_render_surface_release(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, stores_user_set_size_for_driver_to_access)
{
    auto width = 0;
    auto height = 0;
    geom::Size const size { 101, 102 };

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());

    mir_render_surface_get_size(rs, &width, &height);
    EXPECT_THAT(width, Eq(logical_size.width.as_int())); 
    EXPECT_THAT(height, Eq(logical_size.height.as_int()));

    mir_render_surface_set_size(rs, size.width.as_int(), size.height.as_int());
    mir_render_surface_get_size(rs, &width, &height);
    EXPECT_THAT(width, Eq(size.width.as_int())); 
    EXPECT_THAT(height, Eq(size.height.as_int()));

    mir_render_surface_release(rs);
    mir_connection_release(connection);
}
#pragma GCC diagnostic pop
