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

#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/test/doubles/null_display_buffer_compositor_factory.h"
#include "mir/test/signal.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace testing;

namespace
{
void swap_buffers_callback(MirBufferStream*, void* ctx)
{
    auto buffers_swapped = static_cast<mt::Signal*>(ctx);
    buffers_swapped->raise();
}

struct SurfaceSwapBuffers : mtf::ConnectedClientWithASurface
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

        mir_buffer_stream_swap_buffers(mir_surface_get_buffer_stream(surface), swap_buffers_callback, &buffers_swapped);

        /*
         * ASSERT instead of EXPECT, since if we continue we will block in future
         * mir client calls (e.g mir_connection_release).
         */
        ASSERT_TRUE(buffers_swapped.wait_for(std::chrono::seconds{20}));
    }
}
