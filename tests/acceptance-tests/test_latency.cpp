/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
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

#include "mir/graphics/buffer.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/debug/surface.h"
#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/null_display_buffer.h"
#include "mir_test_doubles/null_display_sync_group.h"
#include "mir_test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <unordered_map>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mg = mir::graphics;
namespace
{
/*
 * Note: we're not aiming to check performance in terms of CPU or GPU time processing
 * the incoming buffers. Rather, we're checking that we don't have any intrinsic
 * latency introduced by the swapping algorithms.
 */
struct IdCollectingDB : mtd::NullDisplayBuffer
{
    IdCollectingDB(unsigned int& count) : post_count{count} {}

    mir::geometry::Rectangle view_area() const override
    {
        return {{0,0}, {1920, 1080}};
    } 
    bool post_renderables_if_optimizable(mg::RenderableList const& renderables) override
    {
        /*
         * Clients are blocked only until the below buffer() goes out of
         * scope. Thereafter we'll be racing the client thread. So we need
         * to increment the post_count (represents universal time) here
         * where the client thread is predictably blocked in its call to
         * mir_buffer_stream_swap_buffers_sync().
         */
        ++post_count;

        //the surface will be the frontmost of the renderables
        if (!renderables.empty())
            last = renderables.front()->buffer()->id();
        return true;
    }
    mg::BufferID last_id()
    {
        return last;
    }
private:
    unsigned int& post_count;
    mg::BufferID last{0};
};

struct TimeTrackingGroup : mtd::NullDisplaySyncGroup
{
    TimeTrackingGroup(unsigned int& count, std::unordered_map<uint32_t, uint32_t>& map) :
        post_count(count), timestamps(map), db(count) {}
    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        f(db);
    }

    void post() override
    {
        auto const it = timestamps.find(db.last_id().as_value());
        if (it != timestamps.end())
            latency.push_back(post_count - it->second);
    }

    float average_latency()
    {
        unsigned int sum {0};
        for(auto& s : latency)
            sum += s;

        return static_cast<float>(sum) / latency.size();
    }

    std::vector<int> latency;
    unsigned int& post_count;
    std::unordered_map<uint32_t, uint32_t>& timestamps;
    IdCollectingDB db;
};

struct TimeTrackingDisplay : mtd::NullDisplay
{
    TimeTrackingDisplay(unsigned int& count, std::unordered_map<uint32_t, uint32_t>& map) :
        group(count, map) {}

    void for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f) override
    {
        f(group);
    }
    TimeTrackingGroup group;
};
 
struct ClientLatency : mtf::ConnectedClientWithASurface
{
    void SetUp() override
    {
        preset_display(mt::fake_shared(display));
        mtf::ConnectedClientWithASurface::SetUp();
    }
    unsigned int post_count{0};
    std::unordered_map<uint32_t, uint32_t> timestamps;
    TimeTrackingDisplay display{post_count, timestamps};
    unsigned int test_submissions{100};
};
}

TEST_F(ClientLatency, double_buffered_client_uses_all_buffers)
{
    using namespace testing;

    auto stream = mir_surface_get_buffer_stream(surface);
    for(auto i = 0u; i < test_submissions; i++) {
        auto submission_id = mir_debug_surface_current_buffer_id(surface);
        timestamps[submission_id] = post_count; 
        mir_buffer_stream_swap_buffers_sync(stream);
    }

    unsigned int const expected_client_buffers = 2;
    unsigned int const expected_latency = expected_client_buffers - 1;

    float const error_margin = 0.1f;
    auto observed_latency = display.group.average_latency();
    EXPECT_THAT(observed_latency, AllOf(Gt(expected_latency-error_margin),
                                        Lt(expected_latency+error_margin)));
}

//TODO: configure and add test for triple buffer
