/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "src/server/report/logging/compositor_report.h"
#include "mir/logging/logger.h"
#include <gtest/gtest.h>
#include <string>
#include <cstdio>

using namespace mir;
using namespace std;

namespace
{

class FakeClock : public time::Clock
{
public:
    void elapse(chrono::microseconds delta)
    {
        now += delta;
    }
    time::Timestamp sample() const
    {
        return now;
    }
private:
    time::Timestamp now;
};

class Recorder : public logging::Logger
{
public:
    void log(Severity, string const& message, string const&)
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

} // namespace

TEST(LoggingCompositorReport, calculates_accurate_stats)
{
    /*
     * This test just verifies the important stats; FPS and frame time.
     * We don't need to validate all the numbers because maintaining low
     * coupling to log formats is more important.
     */
    auto clock = make_shared<FakeClock>();
    auto recorder = make_shared<Recorder>();
    const void* const display_id = nullptr;

    report::logging::CompositorReport report(recorder, clock);

    int target_fps = 60;
    for (int frame = 0; frame < target_fps*3; frame++)
    {
        report.began_frame(display_id);
        clock->elapse(chrono::microseconds(1000000 / target_fps));
        report.finished_frame(false, display_id);
    }

    float measured_fps, measured_frame_time;
    ASSERT_TRUE(recorder->scrape(measured_fps, measured_frame_time))
        << recorder->last_message();
    EXPECT_LE(59.9f, measured_fps);
    EXPECT_GE(60.1f, measured_fps);
    EXPECT_LE(16.5f, measured_frame_time);
    EXPECT_GE(16.7f, measured_frame_time);

    clock->elapse(chrono::microseconds(5000000));

    target_fps = 100;
    for (int frame = 0; frame < target_fps*3; frame++)
    {
        report.began_frame(display_id);
        clock->elapse(chrono::microseconds(1000000 / target_fps));
        report.finished_frame(false, display_id);
    }
    ASSERT_TRUE(recorder->scrape(measured_fps, measured_frame_time))
        << recorder->last_message();
    EXPECT_FLOAT_EQ(100.0f, measured_fps);
    EXPECT_FLOAT_EQ(10.0f, measured_frame_time);
}

TEST(LoggingCompositorReport, survives_pause_resume)
{
    auto clock = make_shared<FakeClock>();
    auto logger = make_shared<Recorder>();
    const void* const before = "before";
    const void* const after = "after";

    report::logging::CompositorReport report(logger, clock);

    report.started();

    report.began_frame(before);
    clock->elapse(chrono::microseconds(12345));
    report.finished_frame(false, before);

    report.stopped();
    clock->elapse(chrono::microseconds(12345678));
    report.started();

    report.began_frame(after);
    clock->elapse(chrono::microseconds(12345));
    report.finished_frame(false, after);

    clock->elapse(chrono::microseconds(12345678));

    report.began_frame(after);
    clock->elapse(chrono::microseconds(12345));
    report.finished_frame(false, after);

    report.stopped();
}

TEST(LoggingCompositorReport, reports_bypass_only_when_changed)
{
    const void* const id = "My Screen";

    auto clock = make_shared<FakeClock>();
    auto logger = make_shared<Recorder>();

    report::logging::CompositorReport report(logger, clock);

    report.started();

    report.began_frame(id);
    report.finished_frame(false, id);
    EXPECT_TRUE(logger->last_message_contains("bypass OFF"))
        << logger->last_message();

    for (int f = 0; f < 3; ++f)
    {
        report.began_frame(id);
        report.finished_frame(false, id);
        clock->elapse(chrono::microseconds(12345678));
    }
    EXPECT_FALSE(logger->last_message_contains("bypass "))
        << logger->last_message();

    report.began_frame(id);
    report.finished_frame(true, id);
    EXPECT_TRUE(logger->last_message_contains("bypass ON"))
        << logger->last_message();

    report.stopped();
}
