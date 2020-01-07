/*
 * Copyright © 2016 Canonical Ltd.
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "system_performance_test.h"

using namespace std::literals::chrono_literals;
using namespace mir::test;

namespace
{
struct CompositorPerformance : SystemPerformanceTest
{
    void SetUp() override
    {
        compositor_fps = compositor_render_time = -1.0f;
        SystemPerformanceTest::set_up_with("--compositor-report=log");
    }

    void read_compositor_report()
    {
        char line[256];
        while (fgets(line, sizeof(line), server_output))
        {
            if (char const* perf = strstr(line, "averaged "))
            {
                float fps, render_time;
                if (2 == sscanf(perf, "averaged %f FPS, %f ms/frame",
                                &fps, &render_time))
                {
                    compositor_fps = fps;
                    compositor_render_time = render_time;
                }
            }
        }
    }

    float compositor_fps, compositor_render_time;
};
} // anonymous namespace

TEST_F(CompositorPerformance, regression_test_1563287)
{
    spawn_clients({"mir_demo_client_flicker",
                   "mir_demo_client_egltriangle -b0.5 -f",
                   "mir_demo_client_progressbar",
                   "mir_demo_client_scroll",
                   "mir_demo_client_egltriangle -b0.5",
                   "mir_demo_client_multiwin"});
    run_server_for(10s);

    read_compositor_report();
    EXPECT_GE(compositor_fps, 58.0f);
    EXPECT_LT(compositor_render_time, 17.0f);
}
