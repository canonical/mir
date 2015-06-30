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

#ifndef FRAME_UNIFORMITY_TEST_H_
#define FRAME_UNIFORMITY_TEST_H_

#include "touch_producing_server.h"
#include "touch_measuring_client.h"
#include "touch_samples.h"

#include "mir/geometry/size.h"
#include "mir/geometry/point.h"

#include "mir/test/barrier.h"

#include "mir_test_framework/server_runner.h"

#include <chrono>

struct FrameUniformityTestParameters
{
    mir::geometry::Size screen_size;
    mir::geometry::Point touch_start;
    mir::geometry::Point touch_end;

    std::chrono::milliseconds touch_duration;
};

class FrameUniformityTest : public mir_test_framework::ServerRunner
{
public:
    FrameUniformityTest(FrameUniformityTestParameters const& parameters);
    virtual ~FrameUniformityTest() = default;
    
    mir::DefaultServerConfiguration& server_config() override;
    
    void run_test();
    
    std::shared_ptr<TouchSamples> client_results();
    
    TouchProducingServer::TouchTimings server_timings();

private:
    mir::test::Barrier client_ready_fence;
    TouchProducingServer server_configuration;
    TouchMeasuringClient client;
};

#endif // FRAME_UNIFORMITY_TEST_H_
