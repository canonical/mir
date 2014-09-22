/*
 * Copyright © 2013 Canonical Ltd.
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

#include "mir_test_doubles/mock_display_layout.h"
#include "mir_test_doubles/stub_scene_session.h"

#include "src/server/shell/default_placement_strategy.h"
#include "mir/scene/surface_creation_parameters.h"

#include "mir/geometry/rectangle.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{

struct DefaultPlacementStrategySetup : public testing::Test
{
    void SetUp()
    {
        using namespace ::testing;
        display_layout = std::make_shared<mtd::MockDisplayLayout>();
    }

    std::shared_ptr<mtd::MockDisplayLayout> display_layout;
    mtd::StubSceneSession session;
};
}

TEST_F(DefaultPlacementStrategySetup, parameters_with_output_id_are_placed_in_output)
{
    using namespace ::testing;

    ms::SurfaceCreationParameters input_params;
    input_params.size = geom::Size{100, 200};
    input_params.output_id = mir::graphics::DisplayConfigurationOutputId{1};
    geom::Rectangle rect{input_params.top_left, input_params.size};

    EXPECT_CALL(*display_layout,
                place_in_output(input_params.output_id, rect))
        .Times(1);

    msh::DefaultPlacementStrategy placement_strategy(display_layout);

    placement_strategy.place(session, input_params);
}
