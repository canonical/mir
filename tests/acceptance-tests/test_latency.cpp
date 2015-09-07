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
#include "mir/optional_value.h"
#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/debug/surface.h"
#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/test/doubles/null_display.h"
#include "mir/test/doubles/null_display_buffer.h"
#include "mir/test/doubles/null_display_sync_group.h"
#include "mir/test/fake_shared.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <deque>

namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mg = mir::graphics;
namespace
{

class Stats
{
public:
    void post()
    {
        std::lock_guard<std::mutex> lock{mutex};
        post_count++;
    }

    void record_submission(uint32_t submission_id)
    {
        std::lock_guard<std::mutex> lock{mutex};
        submissions.push_back({submission_id, post_count});
    }

    auto latency_for(uint32_t submission_id)
    {
        std::lock_guard<std::mutex> lock{mutex};

        mir::optional_value<uint32_t> latency;

        for (auto i = submissions.begin(); i != submissions.end(); i++)
        {
            if (i->buffer_id == submission_id)
            {
                latency = post_count - i->time;
                submissions.erase(i);
                break;
            }
        }

        return latency;
    }

private:
    std::mutex mutex;
    unsigned int post_count{0};

    // Note that a buffer_id may appear twice in the list as the client is
    // faster than the compositor and can produce a new frame before the
    // compositor has measured the previous submisson of the same buffer id.
    struct Submission
    {
        uint32_t buffer_id;
        uint32_t time;
    };
    std::deque<Submission> submissions;
};
/*
 * Note: we're not aiming to check performance in terms of CPU or GPU time processing
 * the incoming buffers. Rather, we're checking that we don't have any intrinsic
 * latency introduced by the swapping algorithms.
 */
class IdCollectingDB : public mtd::NullDisplayBuffer
{
public:
    mir::geometry::Rectangle view_area() const override
    {
        return {{0,0}, {1920, 1080}};
    }

    bool post_renderables_if_optimizable(mg::RenderableList const& renderables) override
    {
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
    mg::BufferID last{0};
};

class TimeTrackingGroup : public mtd::NullDisplaySyncGroup
{
public:
    TimeTrackingGroup(Stats& stats) : stats{stats} {}

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        f(db);
    }

    void post() override
    {
        auto latency = stats.latency_for(db.last_id().as_value());
        if (latency.is_set())
        {
            std::lock_guard<std::mutex> lock{mutex};
            latency_list.push_back(latency.value());
        }

        stats.post();

        /*
         * Sleep a little to make the test more realistic. This way the
         * client will actually fill the buffer queue. If we don't do this,
         * then it's like having an infinite refresh rate and the measured
         * latency would never exceed 1.0.  (LP: #1447947)
         */
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    float average_latency()
    {
        std::lock_guard<std::mutex> lock{mutex};

        unsigned int sum {0};
        for(auto& s : latency_list)
            sum += s;

        return static_cast<float>(sum) / latency_list.size();
    }

private:
    std::mutex mutex;
    std::vector<int> latency_list;
    Stats& stats;
    IdCollectingDB db;
};

struct TimeTrackingDisplay : mtd::NullDisplay
{
    TimeTrackingDisplay(Stats& stats)
        : group{stats}
    {
    }

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

    Stats stats;
    TimeTrackingDisplay display{stats};
    unsigned int test_submissions{100};
};
}

TEST_F(ClientLatency, triple_buffered_client_uses_all_buffers)
{
    using namespace testing;

    auto stream = mir_surface_get_buffer_stream(surface);
    for(auto i = 0u; i < test_submissions; i++) {
        auto submission_id = mir_debug_surface_current_buffer_id(surface);
        stats.record_submission(submission_id);
        mir_buffer_stream_swap_buffers_sync(stream);
    }

    // Wait for the compositor to finish rendering all those frames,
    // or else we'll be missing some samples and get a spurious average.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    unsigned int const expected_client_buffers = 3;

    // Note: Using the "early release" optimization without dynamic queue
    //       scaling enabled makes the expected latency possibly up to
    //       nbuffers instead of nbuffers-1. After dynamic queue scaling is
    //       enabled, the average will be lower than this.
    float const expected_max_latency = expected_client_buffers;
    float const expected_min_latency = expected_client_buffers - 1;

    auto observed_latency = display.group.average_latency();

    // We still have a margin for error here. The client and server will
    // be scheduled somewhat unpredictably which affects results. Also
    // affecting results will be the first few frames before the buffer
    // quere is full (during which there will be no buffer latency).
    float const error_margin = 0.1f;

    EXPECT_THAT(observed_latency, Gt(expected_min_latency-error_margin));
    EXPECT_THAT(observed_latency, Lt(expected_max_latency+error_margin));
}
