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

#include "system_performance_test.h"

#include <fstream>
#include <string>

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

        const ::testing::TestInfo *const test_info =
                ::testing::UnitTest::GetInstance()->current_test_info();

        char output_filename[256];
        snprintf(output_filename, sizeof(output_filename) - 1,
                 "/tmp/%s_%s_server.log",
                 test_info->test_case_name(), test_info->name());
        std::ofstream out{output_filename};

        if (out.good())
        {
            std::cerr << "Saving server logs to: " << output_filename << std::endl;
        }
        else
        {
            std::cerr << "Failed to open log file: " << output_filename << std::endl;
        }

        while (fgets(line, sizeof(line), server_output))
        {
            if (out.good())
            {
                out << line;
            }

            if (char const* perf = strstr(line, "averaged "))
            {
                float fps, render_time, latency, frames, seconds;
                if (5 == sscanf(perf, "averaged %f FPS, %f ms/frame, latency %f ms, %f frames over %f sec",
                                &fps, &render_time, &latency, &frames, &seconds))
                {
                    if (frames > 0.0 && seconds > 0.0)
                    {
                        compositor_fps = fps;
                        compositor_render_time = render_time;
                    }
                }
            }
            if (char const* renderer = strstr(line, "GL renderer: "))
            {
                server_renderer.assign(renderer + 13, strlen(renderer) - 14);
            }
            if (char const* mode = strstr(line, "Current mode"))
            {
                server_mode.assign(mode + 13, strlen(mode) - 14);
            }
        }
    }

    float compositor_fps, compositor_render_time;
    std::string server_renderer, server_mode;
};
} // anonymous namespace

TEST_F(CompositorPerformance, regression_test_1563287)
{
    spawn_clients({"mir_demo_client_wayland", "mir_demo_client_wayland_egl_spinner",
                   "mir_demo_client_wayland", "mir_demo_client_wayland_egl_spinner",
                   "mir_demo_client_wayland", "mir_demo_client_wayland_egl_spinner"});
    run_server_for(10s);

    read_compositor_report();
    RecordProperty("framerate", std::to_string(compositor_fps));
    RecordProperty("render_time", std::to_string(compositor_render_time));
    RecordProperty("server_renderer", server_renderer);
    RecordProperty("server_mode", server_mode);
    EXPECT_GE(compositor_fps, 0);
    EXPECT_GT(compositor_render_time, 0);
}
