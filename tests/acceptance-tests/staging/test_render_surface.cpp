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
#include "mir_toolkit/mir_render_surface.h"
#include "mir_toolkit/mir_presentation_chain.h"

#include "mir/geometry/size.h"
#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir/test/validity_matchers.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace geom = mir::geometry;

namespace
{
struct RenderSurfaceTest : mtf::HeadlessInProcessServer
{
    mtf::UsingStubClientPlatform using_stub_client_platform;
    geom::Size const logical_size{640, 480};
};
}

using namespace testing;

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
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware);

    ASSERT_THAT(bs, NotNull());
    EXPECT_TRUE(mir_buffer_stream_is_valid(bs));
    EXPECT_THAT(mir_buffer_stream_get_error_message(bs), StrEq(""));

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
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware);

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
    auto spec = mir_connection_create_spec_for_normal_surface(
        connection,
        physical_size.width.as_int(), physical_size.height.as_int(),
        mir_pixel_format_invalid);
    mir_surface_spec_add_render_surface(
        spec, rs, logical_size.width.as_int(), logical_size.height.as_int(), 0, 0);
    auto surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    EXPECT_THAT(surface, IsValid());
    EXPECT_THAT(mir_surface_get_buffer_stream(surface), Eq(nullptr));

    mir_render_surface_release(rs);
    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, stream_can_be_constructed_after_surface_creation)
{
    int const width{800}, height{600};
    MirPixelFormat const format{mir_pixel_format_abgr_8888};
    MirBufferUsage const usage{mir_buffer_usage_software};

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());
    auto spec = mir_connection_create_spec_for_normal_surface(connection,
                                                              width, height,
                                                              format);
    mir_surface_spec_add_render_surface(spec, rs, width, height, 0, 0);
    auto surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);
    auto bs = mir_render_surface_get_buffer_stream(rs,
                                                   640, 480,
                                                   format,
                                                   usage);

    EXPECT_THAT(surface, IsValid());
    EXPECT_THAT(mir_surface_get_buffer_stream(surface), Eq(nullptr));
    EXPECT_TRUE(mir_buffer_stream_is_valid(bs));

    mir_render_surface_release(rs);
    mir_surface_release_sync(surface);
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

TEST_F(RenderSurfaceTest, chain_can_be_constructed_after_surface_creation)
{
    int const width{800}, height{600};
    MirPixelFormat const format{mir_pixel_format_abgr_8888};

    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    auto rs = mir_connection_create_render_surface_sync(
        connection, logical_size.width.as_int(), logical_size.height.as_int());
    auto spec = mir_connection_create_spec_for_normal_surface(connection,
                                                              width, height,
                                                              format);
    mir_surface_spec_add_render_surface(spec, rs, width, height, 0, 0);
    auto surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);
    auto pc = mir_render_surface_get_presentation_chain(rs);

    EXPECT_THAT(surface, IsValid());
    EXPECT_THAT(mir_surface_get_buffer_stream(surface), Eq(nullptr));
    EXPECT_TRUE(mir_presentation_chain_is_valid(pc));

    mir_render_surface_release(rs);
    mir_surface_release_sync(surface);
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

TEST_F(RenderSurfaceTest, can_hand_out_stream_or_chain_but_not_both)
{
    auto physical_size = logical_size;
    auto connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    {
        auto rs = mir_connection_create_render_surface_sync(
            connection, logical_size.width.as_int(), logical_size.height.as_int());
        auto pc = mir_render_surface_get_presentation_chain(rs);

        ASSERT_THAT(pc, NotNull());
        EXPECT_TRUE(mir_presentation_chain_is_valid(pc));

        auto bs = mir_render_surface_get_buffer_stream(
            rs,
            physical_size.width.as_int(), physical_size.height.as_int(),
            mir_pixel_format_abgr_8888,
            mir_buffer_usage_hardware);

        EXPECT_THAT(bs, Eq(nullptr));

        mir_render_surface_release(rs);
    }

    {
        auto rs = mir_connection_create_render_surface_sync(
            connection, logical_size.width.as_int(), logical_size.height.as_int());
        auto bs = mir_render_surface_get_buffer_stream(
            rs,
            physical_size.width.as_int(), physical_size.height.as_int(),
            mir_pixel_format_abgr_8888,
            mir_buffer_usage_hardware);

        ASSERT_THAT(bs, NotNull());
        EXPECT_TRUE(mir_buffer_stream_is_valid(bs));

        auto pc = mir_render_surface_get_presentation_chain(rs);

        EXPECT_THAT(pc, Eq(nullptr));

        mir_render_surface_release(rs);
    }

    mir_connection_release(connection);
}
