/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#include "mir_test_framework/connected_client_with_a_window.h"
#include "mir/test/doubles/null_display_buffer_compositor_factory.h"
#include "mir/test/signal.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/cursor.h"

#include <gtest/gtest.h>
#include <thread>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
using namespace testing;

namespace
{

struct SurfaceSwapBuffers : mtf::ConnectedClientWithAWindow
{
    void SetUp() override
    {
        server.override_the_display_buffer_compositor_factory([]
        {
            return std::make_shared<mtd::NullDisplayBufferCompositorFactory>();
        });

        ConnectedClientWithAWindow::SetUp();
    }
};

}

TEST_F(SurfaceSwapBuffers, does_not_block_when_surface_is_not_composited)
{
    for (int i = 0; i != 10; ++i)
    {
        mt::Signal buffers_swapped;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        auto bs = mir_window_get_buffer_stream(window);
#pragma GCC diagnostic pop

        /*
         * Since we're using client-side vsync now, it should always return on
         * the swap interval regardless of whether the server actually used
         * the frame.
         */
        std::thread attempt_swap([bs, &buffers_swapped]{
            mir_buffer_stream_swap_buffers_sync(bs);
            buffers_swapped.raise();
        });

        /*
         * ASSERT instead of EXPECT, since if we continue we will block in future
         * mir client calls (e.g mir_connection_release).
         */
        ASSERT_TRUE(buffers_swapped.wait_for(std::chrono::seconds{20}));
        attempt_swap.join();
    }
}

namespace
{
struct BufferCollectingCompositorFactory : mc::DisplayBufferCompositorFactory
{
public:
    auto create_compositor_for(mg::DisplayBuffer&)
        -> std::unique_ptr<mc::DisplayBufferCompositor> override
    {
        struct CollectingDisplayBufferCompositor : mc::DisplayBufferCompositor
        {
            void composite(mc::SceneElementSequence&& seq)
            {
                for (auto& s : seq)
                    buffers.insert(s->renderable()->buffer());
            }
            std::set<std::shared_ptr<mg::Buffer>> buffers;
        };

        return std::make_unique<CollectingDisplayBufferCompositor>();
    }
};

struct SwapBuffersDoesntBlockOnSubmission : mtf::ConnectedClientWithAWindow
{
    unsigned int figure_out_nbuffers()
    {
        auto default_nbuffers = 3u;
        if (auto server_nbuffers_switch = getenv("MIR_SERVER_NBUFFERS"))
        {
            if (server_nbuffers_switch && atoi(server_nbuffers_switch) > 0)
                return atoi(server_nbuffers_switch);
            else
            {
                auto client_nbuffers_switch = getenv("MIR_CLIENT_NBUFFERS");
                if (client_nbuffers_switch)
                    return atoi(client_nbuffers_switch);
            }
        }
        return default_nbuffers;
    }
    void SetUp() override
    {
        server.override_the_display_buffer_compositor_factory([]
        {
            return std::make_shared<BufferCollectingCompositorFactory>();
        });

        ConnectedClientWithAWindow::SetUp();
        server.the_cursor()->hide();
    }

    void TearDown() override
    {
        ConnectedClientWithAWindow::TearDown();
    }

    unsigned int nbuffers = figure_out_nbuffers();
};
}

//LP: #1584784
TEST_F(SwapBuffersDoesntBlockOnSubmission, can_swap_nbuffers_times_without_blocking)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    for (auto i = 0u; i != nbuffers; ++i)
        mir_buffer_stream_swap_buffers(mir_window_get_buffer_stream(window), nullptr, nullptr);
#pragma GCC diagnostic pop
}
