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
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/cursor.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
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

struct SwapBuffersDoesntBlockOnSubmission : mtf::ConnectedClientWithASurface
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

        ConnectedClientWithASurface::SetUp();
        server.the_cursor()->hide();
    }

    void TearDown() override
    {
        ConnectedClientWithASurface::TearDown();
    }

    unsigned int nbuffers = figure_out_nbuffers();
};
}

//LP: #1584784
TEST_F(SwapBuffersDoesntBlockOnSubmission, can_swap_nbuffers_times_without_blocking)
{
    for (auto i = 0u; i != nbuffers; ++i)
        mir_buffer_stream_swap_buffers(mir_surface_get_buffer_stream(surface), nullptr, nullptr);
}
