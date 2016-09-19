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

#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/using_stub_client_platform.h"
#include "mir/test/validity_matchers.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;

namespace
{
struct RenderSurfaceTest : mtf::HeadlessInProcessServer
{
    MirConnection* connection = nullptr;

    mtf::UsingStubClientPlatform using_stub_client_platform;
};
}

using namespace testing;

TEST_F(RenderSurfaceTest, creates_render_surfaces)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto rs = mir_connection_create_render_surface(connection,
        640, 480, mir_pixel_format_abgr_8888);

    ASSERT_THAT(rs, NotNull());
    EXPECT_TRUE(mir_render_surface_is_valid(rs));

    mir_render_surface_release_sync(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, creates_buffer_streams)
{
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto rs = mir_connection_create_render_surface(connection,
        640, 480, mir_pixel_format_abgr_8888);

    ASSERT_THAT(rs, NotNull());
    EXPECT_TRUE(mir_render_surface_is_valid(rs));

    auto bs = mir_render_surface_create_buffer_stream_sync(rs);
    ASSERT_THAT(bs, NotNull());
    EXPECT_TRUE(mir_buffer_stream_is_valid(bs));
    EXPECT_THAT(mir_buffer_stream_get_error_message(bs), StrEq(""));

    mir_buffer_stream_release_sync(bs);
    mir_render_surface_release_sync(rs);
    mir_connection_release(connection);
}

TEST_F(RenderSurfaceTest, render_surfaces_can_be_added_to_spec)
{
    int const width{800}, height{600};
    MirPixelFormat const format{mir_pixel_format_bgr_888};

    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);
    ASSERT_TRUE(mir_connection_is_valid(connection));

    auto rs = mir_connection_create_render_surface(connection,
        width, height, format);

    ASSERT_THAT(rs, NotNull());
    EXPECT_TRUE(mir_render_surface_is_valid(rs));

    auto bs = mir_render_surface_create_buffer_stream_sync(rs);
    ASSERT_THAT(bs, NotNull());
    EXPECT_TRUE(mir_buffer_stream_is_valid(bs));
    EXPECT_THAT(mir_buffer_stream_get_error_message(bs), StrEq(""));

    auto surface_spec = mir_connection_create_spec_for_normal_surface(connection,
                                                                      width, height,
                                                                      format);

    mir_surface_spec_add_render_surface(surface_spec, width, height, 0, 0, rs);

    auto surface = mir_surface_create_sync(surface_spec);
    mir_surface_spec_release(surface_spec);

    EXPECT_THAT(surface, IsValid());

    mir_buffer_stream_release_sync(bs);
    mir_render_surface_release_sync(rs);
    mir_surface_release_sync(surface);
    mir_connection_release(connection);
}
