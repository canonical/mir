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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "frame_uniformity_test.h"

FrameUniformityTest::FrameUniformityTest(FrameUniformityTestParameters const& parameters)
    : client_ready_fence{2},
      server_configuration({{0, 0}, parameters.screen_size},
          parameters.touch_start,
          parameters.touch_end,
          parameters.touch_duration,
          client_ready_fence),
      client(client_ready_fence, parameters.touch_duration)
{
}

mir::DefaultServerConfiguration& FrameUniformityTest::server_config()
{
    return server_configuration;
}

void FrameUniformityTest::run_test()
{
    start_server();
    client.run(new_connection());
    stop_server();
}

std::shared_ptr<TouchSamples> FrameUniformityTest::client_results()
{
    return client.results();
}

TouchProducingServer::TouchTimings FrameUniformityTest::server_timings()
{
    return server_configuration.touch_timings();
}
