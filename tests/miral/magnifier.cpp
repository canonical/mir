/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <miral/magnifier.h>
#include "mir/server.h"
#include "mir/compositor/scene.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"

#include <miral/test_server.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <glm/gtc/matrix_transform.hpp>

namespace mtf = mir_test_framework;

using namespace testing;
using namespace miral;

class MagnifierTest : public TestServer
{
public:
    MagnifierTest()
    {
        start_server_in_setup = false;
        add_to_environment("MIR_SERVER_PLATFORM_DISPLAY_LIBS", "mir:virtual");
        add_to_environment("MIR_SERVER_VIRTUAL_OUTPUT", "800x600");
    }
};

TEST_F(MagnifierTest, magnifier_disabled_by_default)
{
    Magnifier magnifier;
    add_server_init(magnifier);
    add_start_callback([&]
    {
        auto const scene = server().the_scene();
        auto const elements = scene->scene_elements_for(this);
        EXPECT_THAT(elements.size(), Eq(0));
    });
    start_server();
}

TEST_F(MagnifierTest, can_start_enabled)
{
    Magnifier magnifier;
    magnifier.enable(true);
    add_server_init(magnifier);
    add_start_callback([&]
    {
        auto const scene = server().the_scene();
        auto const elements = scene->scene_elements_for(this);
        EXPECT_THAT(elements.size(), Eq(1));
    });
    start_server();
}

TEST_F(MagnifierTest, magnification_results_in_scaled_transform)
{
    Magnifier magnifier;
    magnifier.magnification(2.f);
    add_server_init(magnifier);
    add_start_callback([&]
    {
        magnifier.enable(true);
        auto const scene = server().the_scene();
        auto const elements = scene->scene_elements_for(this);
        EXPECT_THAT(elements.size(), Eq(1));
        auto const expected = glm::mat4{
            1, 0.0, 0.0, 0.0,
            0.0, -1, 0.0, 0.0,
            0.0, 0.0, 1, 0.0,
            0.0, 0.0, 0.0, 1.0
        } * glm::scale(glm::mat4(1.0), glm::vec3(2, 2, 1));

        EXPECT_THAT(elements.at(0)->renderable()->transformation(), Eq(expected));
    });
    start_server();
}

TEST_F(MagnifierTest, can_set_capture_size)
{
    Magnifier magnifier;
    magnifier.capture_size(Size(500, 500));
    add_server_init(magnifier);
    add_start_callback([&]
    {
        magnifier.enable(true);
        auto const scene = server().the_scene();
        auto const elements = scene->scene_elements_for(this);
        EXPECT_THAT(elements.size(), Eq(1));
        EXPECT_THAT(elements.at(0)->renderable()->screen_position().size, Eq(Size(500, 500)));
    });
    start_server();
}
