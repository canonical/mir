/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir/geometry/size.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/buffer.h"
#include "mir/graphics/cursor.h"
#include "mir_test_framework/connected_client_with_a_window.h"
#include "mir/test/doubles/null_display_buffer_compositor_factory.h"
#include "mir/test/signal.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>


namespace mg = mir::graphics;
namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

using namespace testing;

namespace
{
struct SizeEntry
{
    mc::CompositorID id;
    std::vector<geom::Size> physical_sizes;
    std::vector<geom::Size> composited_sizes;
};

class SizeWatcher
{
public:
    void note_renderable_sizes(mc::CompositorID id, mc::SceneElementSequence const& seq)
    {
        std::lock_guard<std::mutex> lk(mutex);
        SizeEntry entry{id, {}, {}};
        for( auto const& element : seq)
        {
            entry.composited_sizes.push_back(element->renderable()->screen_position().size);
            entry.physical_sizes.push_back(element->renderable()->buffer()->size());
        }
        entries.emplace_back(std::move(entry));
        cv.notify_all();
    }

    std::vector<SizeEntry> size_entries()
    {
        std::lock_guard<std::mutex> lk(mutex);
        return entries;
    }

    void wait_for_an_empty_composition()
    {
        std::unique_lock<std::mutex> lk(mutex);
        auto pred = [this] { return !entries.empty() && entries.back().physical_sizes.empty(); };
        if (!cv.wait_for(lk, std::chrono::seconds(2), pred))
            throw std::runtime_error("timeout waiting for empty composition");
    }

    bool wait_until_entries_number_at_least(unsigned int number, std::chrono::seconds timeout)
    {
        std::unique_lock<std::mutex> lk(mutex);
        return cv.wait_for(lk, timeout, [this, number] { return entries.size() >= number; });
    }

    void clear_record()
    {
        std::unique_lock<std::mutex> lk(mutex);
        entries.clear(); 
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<SizeEntry> entries;
};

class SizeWatchingDBCompositorFactory : public mc::DisplayBufferCompositorFactory
{
public:
    SizeWatchingDBCompositorFactory(std::shared_ptr<SizeWatcher> const& watch) :
        watch(watch)
    {
    }

    auto create_compositor_for(mg::DisplayBuffer&)
        -> std::unique_ptr<mc::DisplayBufferCompositor> override
    {
        return std::make_unique<NullDisplayBufferCompositor>(watch);
    }

private:
    struct NullDisplayBufferCompositor : mc::DisplayBufferCompositor
    {
        NullDisplayBufferCompositor(std::shared_ptr<SizeWatcher> const& watch) :
            watch(watch)
        {
        }

        void composite(mc::SceneElementSequence&& seq)
        {
            watch->note_renderable_sizes(this, seq);
            std::this_thread::yield();
        }

        std::shared_ptr<SizeWatcher> const watch;
    };

    std::shared_ptr<SizeWatcher> const watch;
};

struct SurfaceScaling : mtf::ConnectedClientWithAWindow,
                        ::testing::WithParamInterface<int>
{
    SurfaceScaling() :
        watch(std::make_shared<SizeWatcher>())
    {
    }

    void SetUp() override
    {
        server.override_the_display_buffer_compositor_factory([this]
        {
            return std::make_shared<SizeWatchingDBCompositorFactory>(watch);
        });
        ConnectedClientWithAWindow::SetUp();
        server.the_cursor()->hide();

        watch->wait_for_an_empty_composition();
        watch->clear_record();
    }

    std::shared_ptr<SizeWatcher> const watch;
};
}

TEST_P(SurfaceScaling, compositor_sees_size_different_when_scaled)
{
    using namespace std::literals::chrono_literals;
    auto scale = 2.0f;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto stream = mir_window_get_buffer_stream(window);
    mir_buffer_stream_set_swapinterval(stream, GetParam());
    mir_buffer_stream_set_scale(stream, scale);
#pragma GCC diagnostic pop
    mir_buffer_stream_swap_buffers_sync(stream);

    int frames = 0;
    int const max_frames = 30;
    do
    {
        mir_buffer_stream_swap_buffers_sync(stream);
        ++frames;
    } while (!watch->wait_until_entries_number_at_least(2, 1s) &&
             frames < max_frames);

    auto entries = watch->size_entries();
    ASSERT_THAT(entries, SizeIs(Ge(1u)));

    bool an_entry_with_differing_size {false};
    for(auto &entry : entries)
    {
        if (entry.physical_sizes.empty() || entry.composited_sizes.empty())
            continue;
        if (entry.composited_sizes.front() != entry.physical_sizes.front())
            an_entry_with_differing_size = true;
    }
    EXPECT_TRUE(an_entry_with_differing_size);
}

INSTANTIATE_TEST_SUITE_P(PerSwapInterval, SurfaceScaling, ::testing::Values(0,1));
