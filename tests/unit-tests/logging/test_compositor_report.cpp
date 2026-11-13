/*
 * Copyright © Canonical Ltd.
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
 */

#include "src/server/report/logging/compositor_report.h"
#include <mir/logging/logger.h>
#include <mir/test/doubles/advanceable_clock.h>

#include <gtest/gtest.h>
#include <string>
#include <cstdio>
#include <utility>
#include <vector>

using namespace std;

namespace mtd = mir::test::doubles;
namespace mrl = mir::report::logging;
namespace ml = mir::logging;
using namespace std::literals::chrono_literals;

namespace
{

class Recorder : public ml::Logger
{
public:
    void log(ml::Severity, string const& message, string const&)
    {
        last = message;
    }
    string const& last_message() const
    {
        return last;
    }
    bool last_message_contains(char const* substr)
    {
        return last.find(substr) != string::npos;
    }
    bool scrape(float& fps, float& frame_time) const
    {
        return sscanf(last.c_str(), "Display %*s averaged %f FPS, %f ms/frame",
                      &fps, &frame_time) == 2;
    }
private:
    string last;
};

class RecordingCompositorReport : public mrl::CompositorReport
{
public:
    using mrl::CompositorReport::CompositorReport;

    std::vector<std::pair<SubCompositorId, FrameStatistics>> samples;

private:
    void report_statistics(SubCompositorId id, FrameStatistics const& statistics) override
    {
        mrl::CompositorReport::report_statistics(id, statistics);
        samples.emplace_back(id, statistics);
    }
};

struct LoggingCompositorReport : ::testing::Test
{
    std::shared_ptr<mtd::AdvanceableClock> const clock =
        std::make_shared<mtd::AdvanceableClock>();
    std::shared_ptr<Recorder> const recorder =
        make_shared<Recorder>();
    RecordingCompositorReport report{recorder, clock};

    void render_frame(void const* id, chrono::microseconds render_time, chrono::microseconds interval)
    {
        report.scheduled();
        report.began_frame(id);
        clock->advance_by(render_time);
        report.rendered_frame(id);
        clock->advance_by(interval - render_time);
        report.finished_frame(id);
    }
};

} // namespace

TEST_F(LoggingCompositorReport, first_interval_establishes_a_baseline)
{
    render_frame(nullptr, 20ms, 1s);
    EXPECT_TRUE(report.samples.empty());
}

TEST_F(LoggingCompositorReport, reports_structured_interval_totals)
{
    void const* const id = "display";
    render_frame(id, 20ms, 1s);
    render_frame(id, 20ms, 1s);
    render_frame(id, 10ms, 500ms);
    render_frame(id, 10ms, 500ms);

    ASSERT_EQ(report.samples.size(), 2u);
    EXPECT_EQ(report.samples[0].first, id);
    EXPECT_EQ(report.samples[0].second.frames, 1);
    EXPECT_EQ(report.samples[0].second.bypassed_frames, 0);
    EXPECT_EQ(report.samples[0].second.elapsed, 1s);
    EXPECT_EQ(report.samples[0].second.render_time, 20ms);
    EXPECT_EQ(report.samples[0].second.latency, 0us);
    EXPECT_EQ(report.samples[1].second.frames, 2);
    EXPECT_EQ(report.samples[1].second.elapsed, 1s);
    EXPECT_EQ(report.samples[1].second.render_time, 20ms);
}

TEST_F(LoggingCompositorReport, structured_totals_include_latency_and_bypassed_frames)
{
    render_frame(nullptr, 20ms, 1s);

    report.scheduled();
    clock->advance_by(5ms);
    report.began_frame(nullptr);
    clock->advance_by(995ms);
    report.finished_frame(nullptr);

    ASSERT_EQ(report.samples.size(), 1u);
    EXPECT_EQ(report.samples[0].second.frames, 1);
    EXPECT_EQ(report.samples[0].second.bypassed_frames, 1);
    EXPECT_EQ(report.samples[0].second.elapsed, 1s);
    EXPECT_EQ(report.samples[0].second.render_time, 0us);
    EXPECT_EQ(report.samples[0].second.latency, 5ms);
}

TEST_F(LoggingCompositorReport, structured_totals_are_separate_for_each_display)
{
    void const* const first = "first";
    void const* const second = "second";
    render_frame(first, 10ms, 10ms);
    render_frame(second, 20ms, 20ms);
    clock->advance_by(1s);
    render_frame(first, 10ms, 10ms);
    render_frame(second, 20ms, 20ms);
    clock->advance_by(1s);
    render_frame(first, 10ms, 10ms);

    ASSERT_EQ(report.samples.size(), 2u);
    for (auto const& [id, statistics] : report.samples)
    {
        EXPECT_EQ(statistics.frames, 1);
        EXPECT_EQ(statistics.bypassed_frames, 0);
        EXPECT_EQ(statistics.elapsed, 1030ms);
        EXPECT_EQ(statistics.render_time, id == first ? 10ms : 20ms);
    }
    EXPECT_NE(report.samples[0].first, report.samples[1].first);
}

TEST_F(LoggingCompositorReport, structured_totals_handle_an_idle_display)
{
    void const* const active = "active";
    void const* const idle = "idle";
    render_frame(active, 10ms, 10ms);
    render_frame(idle, 20ms, 20ms);
    clock->advance_by(1s);
    render_frame(active, 10ms, 10ms);
    render_frame(active, 10ms, 1s);

    ASSERT_EQ(report.samples.size(), 2u);
    EXPECT_NE(report.samples[0].first, report.samples[1].first);
    for (auto const& [id, statistics] : report.samples)
    {
        if (id == idle)
        {
            EXPECT_EQ(statistics.frames, 0);
            EXPECT_EQ(statistics.bypassed_frames, 0);
            EXPECT_EQ(statistics.elapsed, 0us);
            EXPECT_EQ(statistics.render_time, 0us);
            EXPECT_EQ(statistics.latency, 0us);
        }
        else
        {
            EXPECT_EQ(id, active);
        }
    }
}

TEST_F(LoggingCompositorReport, restarting_discards_previous_structured_totals)
{
    render_frame(nullptr, 20ms, 1s);
    render_frame(nullptr, 20ms, 1s);
    ASSERT_EQ(report.samples.size(), 1u);
    report.samples.clear();

    report.stopped();
    clock->advance_by(5s);
    report.started();
    render_frame(nullptr, 10ms, 1s);
    EXPECT_TRUE(report.samples.empty());
    render_frame(nullptr, 10ms, 1s);

    ASSERT_EQ(report.samples.size(), 1u);
    EXPECT_EQ(report.samples[0].second.frames, 1);
    EXPECT_EQ(report.samples[0].second.elapsed, 1s);
    EXPECT_EQ(report.samples[0].second.render_time, 10ms);
}

TEST_F(LoggingCompositorReport, calculates_accurate_stats)
{
    /*
     * This test just verifies the important stats; FPS and frame time.
     * We don't need to validate all the numbers because maintaining low
     * coupling to log formats is more important.
     */
    const void* const display_id = nullptr;

    int target_fps = 60;
    for (int frame = 0; frame < target_fps*3; frame++)
    {
        report.began_frame(display_id);
        clock->advance_by(chrono::microseconds(1000000 / target_fps));
        report.rendered_frame(display_id);
        report.finished_frame(display_id);
    }

    float measured_fps, measured_frame_time;
    ASSERT_TRUE(recorder->scrape(measured_fps, measured_frame_time))
        << recorder->last_message();
    EXPECT_LE(59.9f, measured_fps);
    EXPECT_GE(60.1f, measured_fps);
    EXPECT_LE(16.5f, measured_frame_time);
    EXPECT_GE(16.7f, measured_frame_time);

    clock->advance_by(chrono::microseconds(5000000));

    target_fps = 100;
    for (int frame = 0; frame < target_fps*3; frame++)
    {
        report.began_frame(display_id);
        clock->advance_by(chrono::microseconds(1000000 / target_fps));
        report.rendered_frame(display_id);
        report.finished_frame(display_id);
    }
    ASSERT_TRUE(recorder->scrape(measured_fps, measured_frame_time))
        << recorder->last_message();
    EXPECT_FLOAT_EQ(100.0f, measured_fps);
    EXPECT_FLOAT_EQ(10.0f, measured_frame_time);
}

TEST_F(LoggingCompositorReport, survives_pause_resume)
{
    const void* const before = "before";
    const void* const after = "after";

    report.started();

    report.began_frame(before);
    clock->advance_by(chrono::microseconds(12345));
    report.rendered_frame(before);
    report.finished_frame(before);

    report.stopped();
    clock->advance_by(chrono::microseconds(12345678));
    report.started();

    report.began_frame(after);
    clock->advance_by(chrono::microseconds(12345));
    report.rendered_frame(after);
    report.finished_frame(after);

    clock->advance_by(chrono::microseconds(12345678));

    report.began_frame(after);
    clock->advance_by(chrono::microseconds(12345));
    report.rendered_frame(after);
    report.finished_frame(after);

    report.stopped();
}

TEST_F(LoggingCompositorReport, reports_bypass_only_when_changed)
{
    const void* const id = "My Screen";

    report.started();

    report.began_frame(id);
    report.rendered_frame(id);
    report.finished_frame(id);
    EXPECT_TRUE(recorder->last_message_contains("bypass OFF"))
        << recorder->last_message();

    for (int f = 0; f < 3; ++f)
    {
        report.began_frame(id);
        report.rendered_frame(id);
        report.finished_frame(id);
        clock->advance_by(chrono::microseconds(12345678));
    }
    EXPECT_FALSE(recorder->last_message_contains("bypass "))
        << recorder->last_message();

    report.began_frame(id);
    report.finished_frame(id);
    EXPECT_TRUE(recorder->last_message_contains("bypass ON"))
        << recorder->last_message();

    report.stopped();
}

TEST_F(LoggingCompositorReport, bypass_has_no_render_time)
{  // Regression test for LP: #1408906
    const void* const id = "My Screen";

    report.started();

    for (int f = 0; f < 3; ++f)
    {
        report.began_frame(id);
        clock->advance_by(chrono::microseconds(1234));
        report.finished_frame(id);
        clock->advance_by(chrono::microseconds(12345678));
    }
    EXPECT_TRUE(recorder->last_message_contains("0.000 ms/frame"))
        << recorder->last_message();

    report.stopped();
}
