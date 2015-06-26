/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "src/client/periodic_perf_report.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <chrono>

#include "mir/test/doubles/advanceable_clock.h"

using namespace mir;

namespace mtd = mir::test::doubles;

namespace
{

class MockPeriodicPerfReport : public client::PeriodicPerfReport
{
public:
    MockPeriodicPerfReport(mir::time::Duration period,
                           std::shared_ptr<mir::time::Clock> const& clock)
        : client::PeriodicPerfReport(period, clock)
    {
    }

    MOCK_CONST_METHOD5(display, void(const char*,long,long,long,int));
};

struct PeriodicPerfReport : ::testing::Test
{
    std::chrono::seconds const period{1};
    std::shared_ptr<mtd::AdvanceableClock> const clock = std::make_shared<mtd::AdvanceableClock>();
    MockPeriodicPerfReport report{period, clock};
};

} // namespace

TEST_F(PeriodicPerfReport, reports_the_right_numbers_at_full_speed)
{
    int const fps = 50;
    int const nbuffers = 3;
    std::chrono::microseconds const render_time = std::chrono::milliseconds(3);
    auto const frame_time = std::chrono::microseconds(1000000 / fps);
    const char* const name = "Foo";

    report.name_surface(name);

    int const nframes = 1000;
    long const expected_render_time = render_time.count();
    long const expected_lag =
        nbuffers * frame_time.count() - expected_render_time;

    using namespace testing;

    int const nreports = nframes / (period.count() * fps);
    EXPECT_CALL(report, display(StrEq(name),
                                fps*100,
                                expected_render_time,
                                Le(expected_lag), // first report is less
                                nbuffers))
                .Times(1);
    EXPECT_CALL(report, display(StrEq(name),
                                fps*100,
                                expected_render_time,
                                expected_lag, // exact, after first report
                                nbuffers))
                .Times(nreports - 1);

    for (int f = 0; f < nframes; ++f)
    {
        int const buffer_id = f % nbuffers;

        clock->advance_by(frame_time - render_time);
        report.begin_frame(buffer_id);
        clock->advance_by(render_time);
        report.end_frame(buffer_id);
    }
}

TEST_F(PeriodicPerfReport, reports_the_right_numbers_at_low_speed)
{
    int const nbuffers = 3;
    std::chrono::microseconds const render_time = std::chrono::milliseconds(3);
    auto const frame_time = std::chrono::seconds(4);
    const char* const name = "Foo";

    report.name_surface(name);

    using namespace testing;

    int const nframes = 7;

    EXPECT_CALL(report, display(StrEq(name),
                                100/frame_time.count(),
                                render_time.count(),
                                _,
                                _))
                .Times(nframes);

    for (int f = 0; f < nframes; ++f)
    {
        int const buffer_id = f % nbuffers;
        clock->advance_by(frame_time - render_time);
        report.begin_frame(buffer_id);
        clock->advance_by(render_time);
        report.end_frame(buffer_id);
    }
}

TEST_F(PeriodicPerfReport, reports_nothing_on_idle)
{
    using namespace testing;
    EXPECT_CALL(report, display(_,_,_,_,_)).Times(0);
    clock->advance_by(std::chrono::seconds(10));
}

