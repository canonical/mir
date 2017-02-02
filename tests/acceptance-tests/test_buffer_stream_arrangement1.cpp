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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "buffer_stream_arrangement.h"
#include "mir_toolkit/mir_presentation_chain.h"
#include "mir_toolkit/rs/mir_render_surface.h"
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace geom = mir::geometry;
using namespace std::literals::chrono_literals;
using namespace testing;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
namespace
{

struct RenderSurface
{
    RenderSurface(MirConnection* connection, geom::Size size) :
        surface(mir_connection_create_render_surface_sync(
            connection, size.width.as_int(), size.height.as_int()))
    {
    }

    ~RenderSurface()
    {
        mir_render_surface_release(surface);
    }
    MirRenderSurface* surface;
};
struct RenderSurfaceBasedStream : mt::Stream
{
    RenderSurfaceBasedStream(
        MirConnection* connection,
        std::unique_ptr<RenderSurface> rs,
        geom::Size physical_size,
        geom::Rectangle position) :
        mt::Stream(position, [&] {
            return mir_render_surface_get_buffer_stream(
                rs->surface,
                physical_size.width.as_int(),
                physical_size.height.as_int(),
                an_available_format(connection));
        }),
        rs(std::move(rs))
    {
    }

    std::unique_ptr<RenderSurface> const rs;
};

}

typedef mt::BufferStreamArrangementBase BufferStreamArrangementStaging;
TEST_F(BufferStreamArrangementStaging, can_set_stream_logical_and_physical_size)
{
    geom::Size logical_size { 233, 111 };
    geom::Size physical_size { 133, 222 };
    RenderSurfaceBasedStream stream(
        connection,
        std::make_unique<RenderSurface>(connection, physical_size),
        physical_size,
        { {0, 0}, logical_size });

    auto change_spec = mir_create_window_spec(connection);
    mir_window_spec_add_render_surface(change_spec, stream.rs->surface,
        logical_size.width.as_int(), logical_size.height.as_int(), 0, 0);
    mir_window_apply_spec(window, change_spec);
    mir_window_spec_release(change_spec);

    std::vector<mt::RelativeRectangle> positions { { {0,0}, logical_size, physical_size } };
    EXPECT_TRUE(ordering->wait_for_positions_within(positions, 5s))
         << "timed out waiting to see the compositor post the streams in the right arrangement";
}

TEST_F(BufferStreamArrangementStaging, can_setting_stream_physical_size_doesnt_affect_logical)
{
    geom::Size logical_size { 233, 111 };
    geom::Size original_physical_size { 133, 222 };
    geom::Size changed_physical_size { 133, 222 };
    RenderSurfaceBasedStream stream(
        connection,
        std::make_unique<RenderSurface>(connection, original_physical_size),
        original_physical_size,
        { {0, 0}, logical_size });

    auto change_spec = mir_create_window_spec(connection);
    mir_window_spec_add_render_surface(change_spec, stream.rs->surface,
        logical_size.width.as_int(), logical_size.height.as_int(), 0, 0);
    mir_window_apply_spec(window, change_spec);
    mir_window_spec_release(change_spec);

    stream.set_size(changed_physical_size);
    //submits the original_buffer_size buffer, getting changed_physical_size buffer as current
    stream.swap_buffers();
    //submits the changed_buffer_size buffer so server can see
    stream.swap_buffers();

    std::vector<mt::RelativeRectangle> positions { { {0,0}, logical_size, changed_physical_size } };
    EXPECT_TRUE(ordering->wait_for_positions_within(positions, 5s))
         << "timed out waiting to see the compositor post the streams in the right arrangement";
}

TEST_F(BufferStreamArrangementStaging, stream_size_reflects_current_buffer_physical_size)
{
    geom::Size logical_size { 233, 111 };
    geom::Size original_physical_size { 133, 222 };
    geom::Size changed_physical_size { 133, 222 };
    RenderSurfaceBasedStream stream(
        connection,
        std::make_unique<RenderSurface>(connection, original_physical_size),
        original_physical_size,
        { {0, 0}, logical_size });

    auto change_spec = mir_create_window_spec(connection);
    mir_window_spec_add_render_surface(change_spec, stream.rs->surface,
        logical_size.width.as_int(), logical_size.height.as_int(), 0, 0);
    mir_window_apply_spec(window, change_spec);
    mir_window_spec_release(change_spec);

    EXPECT_THAT(stream.physical_size(), Eq(original_physical_size));
    streams.back()->set_size(changed_physical_size);
    EXPECT_THAT(stream.physical_size(), Eq(changed_physical_size));
    streams.back()->swap_buffers();
    EXPECT_THAT(stream.physical_size(), Eq(changed_physical_size));
}
#pragma GCC diagnostic pop
