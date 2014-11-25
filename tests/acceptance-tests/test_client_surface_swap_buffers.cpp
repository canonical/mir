/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_doubles/null_display_buffer_compositor_factory.h"
#include "mir_test/signal.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
void swap_buffers_callback(MirSurface*, void* ctx)
{
    auto buffers_swapped = static_cast<mt::Signal*>(ctx);
    buffers_swapped->raise();
}

struct ConnectedClientWithASurface : mtf::ConnectedClientHeadlessServer
{
    MirSurface* surface{nullptr};

    void SetUp() override
    {
        mtf::ConnectedClientHeadlessServer::SetUp();

        MirSurfaceParameters request_params =
        {
            __PRETTY_FUNCTION__,
            640, 480,
            mir_pixel_format_abgr_8888,
            mir_buffer_usage_hardware,
            mir_display_output_id_invalid
        };

        surface = mir_connection_create_surface_sync(connection, &request_params);
        ASSERT_TRUE(mir_surface_is_valid(surface));
    }

    void TearDown() override
    {
        mir_surface_release_sync(surface);
        mtf::ConnectedClientHeadlessServer::TearDown();
    }
};

struct SurfaceSwapBuffers : ConnectedClientWithASurface
{
    void SetUp() override
    {
        server.override_the_display_buffer_compositor_factory([]
        {
            return std::make_shared<mtd::NullDisplayBufferCompositorFactory>();
        });

        ConnectedClientWithASurface::SetUp();
    }
};
}

TEST_F(SurfaceSwapBuffers, does_not_block_when_surface_is_not_composited)
{
    for (int i = 0; i != 10; ++i)
    {
        mt::Signal buffers_swapped;

        mir_surface_swap_buffers(surface, swap_buffers_callback, &buffers_swapped);

        /*
         * ASSERT instead of EXPECT, since if we continue we will block in future
         * mir client calls (e.g mir_connection_release).
         */
        ASSERT_TRUE(buffers_swapped.wait_for(std::chrono::seconds{5}));
    }
}
