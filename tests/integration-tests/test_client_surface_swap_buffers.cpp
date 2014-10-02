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

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"
//#include "mir_test_doubles/null_display_buffer_compositor_factory.h"
#include "mir_test/signal.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
//namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;

//namespace
//{
//
//struct StubServerConfig : mtf::StubbedServerConfiguration
//{
//    auto the_display_buffer_compositor_factory()
//        -> std::shared_ptr<mc::DisplayBufferCompositorFactory> override
//    {
//        return std::make_shared<mtd::NullDisplayBufferCompositorFactory>();
//    }
//};
//
//void swap_buffers_callback(MirSurface*, void* ctx)
//{
//    auto buffers_swapped = static_cast<mt::Signal*>(ctx);
//    buffers_swapped->raise();
//}
//
//using MirSurfaceSwapBuffersTest = mtf::BasicClientServerFixture<StubServerConfig>;
//
//}
//
//TEST_F(MirSurfaceSwapBuffersTest, swap_buffers_does_not_block_when_surface_is_not_composited)
//{
//    using namespace testing;
//
//    MirSurfaceParameters request_params =
//    {
//        __PRETTY_FUNCTION__,
//        640, 480,
//        mir_pixel_format_abgr_8888,
//        mir_buffer_usage_hardware,
//        mir_display_output_id_invalid
//    };
//
//    auto const surface = mir_connection_create_surface_sync(connection, &request_params);
//    ASSERT_TRUE(mir_surface_is_valid(surface));
//
//    for (int i = 0; i < 10; ++i)
//    {
//        mt::Signal buffers_swapped;
//
//        mir_surface_swap_buffers(surface, swap_buffers_callback, &buffers_swapped);
//
//        /*
//         * ASSERT instead of EXPECT, since if we continue we will block in future
//         * mir client calls (e.g mir_connection_release).
//         */
//        ASSERT_TRUE(buffers_swapped.wait_for(std::chrono::seconds{5}));
//    }
//
//    mir_surface_release_sync(surface);
//}
