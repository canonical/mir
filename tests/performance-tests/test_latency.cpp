/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
#include "mir_test_framework/connected_client_with_a_window.h"
#include "mir/test/doubles/null_display.h"
#include "mir/test/doubles/null_display_buffer.h"
#include "mir/test/doubles/null_display_sync_group.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/fake_shared.h"
#include "mir_test_framework/visible_surface.h"
#include "mir/options/option.h"
#include "mir/test/doubles/null_logger.h"  // for mtd::logging_opt

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <deque>

#include <mutex>
#include <condition_variable>

using namespace ::std::chrono;
using namespace ::std::chrono_literals;
using namespace ::testing;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mg = mir::graphics;

namespace
{

unsigned int const expected_client_buffers = 3;
int const refresh_rate = 60;
microseconds const vblank_interval(1000000/refresh_rate);

class Stats
{
public:
    void post()
    {
        std::lock_guard<std::mutex> lock{mutex};
        visible_frame++;
        posted.notify_one();
    }

    void record_submission(uint32_t submission_id)
    {
        std::lock_guard<std::mutex> lock{mutex};
        submissions.push_back({submission_id, visible_frame});
    }

    auto latency_for(uint32_t submission_id)
    {
        std::lock_guard<std::mutex> lock{mutex};

        mir::optional_value<float> latency;
        int buffers_within_one_frame = 0;

        for (auto i = submissions.begin(); i != submissions.end();)
        {
            if (i->buffer_id == submission_id)
            {
                // The server is skipping a frame we gave it.
                // Fix it up so our stats aren't skewed by the miss:
                if (i != submissions.begin())
                    i = submissions.erase(submissions.begin(), i);

                unsigned frames = visible_frame - i->visible_frame_when_submitted;
                latency = frames;
                if (frames == 1)
                    ++buffers_within_one_frame;

                i = submissions.erase(i);
                if (swap_interval > 0)
                    break;
                //else for interval zero delete dropped entries and use last
            }
            else
            {
                i++;
            }
        }

        if (buffers_within_one_frame > 1)
            latency = latency.value() -
                      (1.0f - (1.0f / buffers_within_one_frame));

        return latency;
    }

    unsigned int frames_composited() const
    {
        std::lock_guard<std::mutex> lock(mutex);
        return visible_frame;
    }

    unsigned int swap_interval{1};

private:
    mutable std::mutex mutex;
    std::condition_variable posted;
    unsigned int visible_frame{0};

    // Note that a buffer_id may appear twice in the list as the client is
    // faster than the compositor and can produce a new frame before the
    // compositor has measured the previous submisson of the same buffer id.
    struct Submission
    {
        uint32_t buffer_id;
        unsigned int visible_frame_when_submitted;
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

    bool overlay(mg::RenderableList const& renderables) override
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
        stats.post();

        auto latency = stats.latency_for(db.last_id().as_value());
        if (latency.is_set())
        {
            std::lock_guard<std::mutex> lock{mutex};
            latency_list.push_back(latency.value());
            if (latency.value() > max)
                max = latency.value();
        }

        /*
         * Sleep a little to make the test more realistic. This way the
         * client will actually fill the buffer queue. If we don't do this,
         * then it's like having an infinite refresh rate and the measured
         * latency would never exceed 1.0.  (LP: #1447947)
         */
        std::this_thread::sleep_for(vblank_interval);
    }

    float average_latency()
    {
        std::lock_guard<std::mutex> lock{mutex};

        float sum = 0.0f;
        for(auto& s : latency_list)
            sum += s;

        return sum / latency_list.size();
    }

    void dump_latency()
    {
        FILE* file = stdout;  // gtest seems to use this
        char const prefix[] = "[  debug   ] ";
        unsigned const size = latency_list.size();
        unsigned const cols = 10u;

        fprintf(file, "%s%u frames sampled, averaging %.3f frames latency\n",
                prefix, size, average_latency());
        for (unsigned i = 0; i < size; ++i)
        {
            if ((i % cols) == 0)
                fprintf(file, "%s%2u:", prefix, i);
            fprintf(file, " %5.2f", latency_list[i]);
            if ((i % cols) == cols-1)
                fprintf(file, "\n");
        }
        if (size % cols)
            fprintf(file, "\n");
    }

    float max_latency() const
    {
        return max;
    }

private:
    std::mutex mutex;
    std::vector<float> latency_list;
    float max = 0;
    Stats& stats;
    IdCollectingDB db;
};

struct TimeTrackingDisplay : mtd::NullDisplay
{
    TimeTrackingDisplay(Stats& stats)
        : group{stats}
    {
        mg::DisplayConfigurationOutput output{
            mg::DisplayConfigurationOutputId{1},
            mg::DisplayConfigurationCardId{1},
            mg::DisplayConfigurationOutputType::hdmia,
            std::vector<MirPixelFormat>{mir_pixel_format_abgr_8888},
            {{{1920,1080}, refresh_rate}},  // <=== Refresh rate must be valid
            0, mir::geometry::Size{}, true, true, mir::geometry::Point{}, 0,
            mir_pixel_format_abgr_8888, mir_power_mode_on,
            mir_orientation_normal,
            1.0f,
            mir_form_factor_monitor,
            mir_subpixel_arrangement_unknown,
            {},
            mir_output_gamma_unsupported,
            {},
            {}
        };
        outputs.push_back(output);
    }

    std::unique_ptr<mg::DisplayConfiguration> configuration() const override
    {
        return std::make_unique<mtd::StubDisplayConfig>(outputs);
    }

    void for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& f) override
    {
        f(group);
    }

    std::vector<mg::DisplayConfigurationOutput> outputs;
    TimeTrackingGroup group;
};
 
struct ClientLatency : mtf::ConnectedClientHeadlessServer
{
    void SetUp() override
    {
        preset_display(mt::fake_shared(display));
        mtf::ConnectedClientHeadlessServer::SetUp();

        auto del = [] (MirWindowSpec* spec) { mir_window_spec_release(spec); };
        std::unique_ptr<MirWindowSpec, decltype(del)> spec(
            mir_create_normal_window_spec(connection, 100, 100),
            del);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_window_spec_set_pixel_format(spec.get(), mir_pixel_format_abgr_8888);
#pragma GCC diagnostic pop
        visible_surface = std::make_unique<mtf::VisibleSurface>(spec.get());
        surface =  *visible_surface;

        stats.swap_interval = 1;
    }

    void TearDown() override
    {
        visible_surface.reset();
        mtf::ConnectedClientHeadlessServer::TearDown();
    }

    Stats stats;
    TimeTrackingDisplay display{stats};
    unsigned int test_frames{100};

    // We still have a margin for error here. The client and server will
    // be scheduled somewhat unpredictably which affects results. Also
    // affecting results will be the first few frames before the buffer
    // quere is full (during which there will be no buffer latency).
    float const error_margin = 0.4f;
    std::unique_ptr<mtf::VisibleSurface> visible_surface;
    MirWindow* surface;
};
}

TEST_F(ClientLatency, average_swap_buffers_sync_latency_is_one_frame)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto stream = mir_window_get_buffer_stream(surface);
#pragma GCC diagnostic pop
    auto const deadline = steady_clock::now() + 60s;

    while (stats.frames_composited() < test_frames &&
           steady_clock::now() < deadline)
    {
        auto submission_id = mir_debug_window_current_buffer_id(surface);
        stats.record_submission(submission_id);
        mir_buffer_stream_swap_buffers_sync(stream);
    }

    ASSERT_THAT(stats.frames_composited(), Ge(test_frames));

    if (server.get_options()->get<bool>(mtd::logging_opt))
        display.group.dump_latency();

    auto average_latency = display.group.average_latency();

    EXPECT_NEAR(1.0f, average_latency, error_margin);
}

TEST_F(ClientLatency, manual_vsync_latency_is_one_frame)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto stream = mir_window_get_buffer_stream(surface);
#pragma GCC diagnostic pop
    mir_buffer_stream_set_swapinterval(stream, 0);
    stats.swap_interval = 0;

    auto const deadline = steady_clock::now() + 60s;

    while (stats.frames_composited() < test_frames &&
           steady_clock::now() < deadline)
    {
        auto const us = mir_buffer_stream_get_microseconds_till_vblank(stream);
        std::this_thread::sleep_for(microseconds(us));

        auto submission_id = mir_debug_window_current_buffer_id(surface);
        stats.record_submission(submission_id);
        mir_buffer_stream_swap_buffers_sync(stream);
    }

    ASSERT_THAT(stats.frames_composited(), Ge(test_frames));

    if (server.get_options()->get<bool>(mtd::logging_opt))
        display.group.dump_latency();

    auto average_latency = display.group.average_latency();

    EXPECT_NEAR(1.0f, average_latency, error_margin);
}

TEST_F(ClientLatency, max_latency_is_limited_to_nbuffers)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto stream = mir_window_get_buffer_stream(surface);
#pragma GCC diagnostic pop
    auto const deadline = steady_clock::now() + 60s;

    while (stats.frames_composited() < test_frames &&
           steady_clock::now() < deadline)
    {
        auto submission_id = mir_debug_window_current_buffer_id(surface);
        stats.record_submission(submission_id);
        mir_buffer_stream_swap_buffers_sync(stream);
    }

    ASSERT_THAT(stats.frames_composited(), Ge(test_frames));

    auto max_latency = display.group.max_latency();
    EXPECT_THAT(max_latency, Le(expected_client_buffers));
}

TEST_F(ClientLatency, dropping_latency_is_closer_to_zero_than_one)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto stream = mir_window_get_buffer_stream(surface);
#pragma GCC diagnostic pop
    mir_buffer_stream_set_swapinterval(stream, 0);
    stats.swap_interval = 0;
    auto const deadline = steady_clock::now() + 60s;

    while (stats.frames_composited() < test_frames &&
           steady_clock::now() < deadline)
    {
        auto submission_id = mir_debug_window_current_buffer_id(surface);
        stats.record_submission(submission_id);
        mir_buffer_stream_swap_buffers_sync(stream);
    }

    ASSERT_THAT(stats.frames_composited(), Ge(test_frames));

    auto average_latency = display.group.average_latency();
    EXPECT_THAT(average_latency, Lt(0.5f));

    if (server.get_options()->get<bool>(mtd::logging_opt))
        display.group.dump_latency();
}

TEST_F(ClientLatency, average_async_swap_latency_is_less_than_nbuffers)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto stream = mir_window_get_buffer_stream(surface);
#pragma GCC diagnostic pop
    auto const deadline = steady_clock::now() + 60s;

    while (stats.frames_composited() < test_frames &&
           steady_clock::now() < deadline)
    {
        auto submission_id = mir_debug_window_current_buffer_id(surface);
        stats.record_submission(submission_id);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_wait_for(mir_buffer_stream_swap_buffers(stream, NULL, NULL));
#pragma GCC diagnostic pop
    }

    ASSERT_THAT(stats.frames_composited(), Ge(test_frames));

    if (server.get_options()->get<bool>(mtd::logging_opt))
        display.group.dump_latency();

    auto average_latency = display.group.average_latency();

    EXPECT_THAT(average_latency, Lt(expected_client_buffers));
}

TEST_F(ClientLatency, max_async_swap_latency_is_limited_to_nbuffers)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto stream = mir_window_get_buffer_stream(surface);
#pragma GCC diagnostic pop
    auto const deadline = steady_clock::now() + 60s;

    while (stats.frames_composited() < test_frames &&
           steady_clock::now() < deadline)
    {
        auto submission_id = mir_debug_window_current_buffer_id(surface);
        stats.record_submission(submission_id);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_wait_for(mir_buffer_stream_swap_buffers(stream, NULL, NULL));
#pragma GCC diagnostic pop
    }

    ASSERT_THAT(stats.frames_composited(), Ge(test_frames));

    auto max_latency = display.group.max_latency();
    EXPECT_THAT(max_latency, Le(expected_client_buffers));
}

TEST_F(ClientLatency, async_swap_dropping_latency_is_closer_to_zero_than_one)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto stream = mir_window_get_buffer_stream(surface);
#pragma GCC diagnostic pop
    mir_buffer_stream_set_swapinterval(stream, 0);
    stats.swap_interval = 0;
    auto const deadline = steady_clock::now() + 60s;

    while (stats.frames_composited() < test_frames &&
           steady_clock::now() < deadline)
    {
        auto submission_id = mir_debug_window_current_buffer_id(surface);
        stats.record_submission(submission_id);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        mir_wait_for(mir_buffer_stream_swap_buffers(stream, NULL, NULL));
#pragma GCC diagnostic pop
    }

    ASSERT_THAT(stats.frames_composited(), Ge(test_frames));

    auto average_latency = display.group.average_latency();
    EXPECT_THAT(average_latency, Lt(0.5f));

    if (server.get_options()->get<bool>(mtd::logging_opt))
        display.group.dump_latency();
}

TEST_F(ClientLatency, throttled_input_rate_yields_lower_latency)
{
    int const throttled_input_rate = refresh_rate * 3 / 4;
    microseconds const input_interval(1000000/throttled_input_rate);
    auto next_input_event = high_resolution_clock::now();
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto stream = mir_window_get_buffer_stream(surface);
#pragma GCC diagnostic pop
    auto const deadline = steady_clock::now() + 60s;

    while (stats.frames_composited() < test_frames &&
           steady_clock::now() < deadline)
    {
        std::this_thread::sleep_until(next_input_event);
        next_input_event += input_interval;

        auto submission_id = mir_debug_window_current_buffer_id(surface);
        stats.record_submission(submission_id);
        mir_buffer_stream_swap_buffers_sync(stream);
    }

    ASSERT_THAT(stats.frames_composited(), Ge(test_frames));

    if (server.get_options()->get<bool>(mtd::logging_opt))
        display.group.dump_latency();

    // As the client is producing frames slower than the compositor consumes
    // them, the buffer queue never fills. So latency is low;
    float const observed_latency = display.group.average_latency();
    EXPECT_THAT(observed_latency, Ge(0.0f));
    EXPECT_THAT(observed_latency, Le(1.0f + error_margin));
}
