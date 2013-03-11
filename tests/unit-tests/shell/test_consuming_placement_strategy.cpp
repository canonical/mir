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

#include "mir_test_doubles/mock_viewable_area.h"

#include "mir/shell/consuming_placement_strategy.h"
#include "mir/shell/surface_creation_parameters.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{
static const geom::Rectangle default_view_area = geom::Rectangle{geom::Point(),
                                                                 geom::Size{geom::Width(1600),
                                                                            geom::Height(1600)}};

struct ConsumingPlacementStrategySetup : public testing::Test
{
    void SetUp()
    {
        using namespace ::testing;
        viewable_area = std::make_shared<mtd::MockViewableArea>();
        ON_CALL(*viewable_area, view_area()).WillByDefault(Return(default_view_area));
    }

    std::shared_ptr<mtd::MockViewableArea> viewable_area;
};
}


TEST_F(ConsumingPlacementStrategySetup, parameters_with_no_geometry_receieve_geometry_from_viewable_area)
{
    using namespace ::testing;

    EXPECT_CALL(*viewable_area, view_area()).Times(1);

    msh::ConsumingPlacementStrategy placement_strategy(viewable_area);
    msh::SurfaceCreationParameters input_params;
    
    auto placed_params = placement_strategy.place(input_params);
    EXPECT_EQ(default_view_area.size.width.as_uint32_t(), placed_params.size.width.as_uint32_t());
    EXPECT_EQ(default_view_area.size.height.as_uint32_t(), placed_params.size.height.as_uint32_t());
}

TEST_F(ConsumingPlacementStrategySetup, parameters_with_geometry_are_forwarded)
{
    using namespace ::testing;
    
    const geom::Size requested_size = geom::Size{geom::Width{100}, geom::Height{100}};

    EXPECT_CALL(*viewable_area, view_area()).Times(1);

    msh::ConsumingPlacementStrategy placement_strategy(viewable_area);
    msh::SurfaceCreationParameters input_params;
    
    input_params.size = requested_size;
    
    auto placed_params = placement_strategy.place(input_params);
    EXPECT_EQ(requested_size, placed_params.size);
}

TEST_F(ConsumingPlacementStrategySetup, parameters_with_unreasonable_geometry_are_clipped)
{
    using namespace ::testing;
    
    const geom::Width unreasonable_width = geom::Width{default_view_area.size.width.as_uint32_t() + 1};
    const geom::Height unreasonable_height = geom::Height{default_view_area.size.width.as_uint32_t() + 1};
    const geom::Size unreasonable_size = geom::Size{unreasonable_width, unreasonable_height};

    EXPECT_CALL(*viewable_area, view_area()).Times(1);
    msh::ConsumingPlacementStrategy placement_strategy(viewable_area);
    msh::SurfaceCreationParameters input_params;
    
    input_params.size = unreasonable_size;

    auto placed_params = placement_strategy.place(input_params);
    
    auto const& clipped_size = default_view_area.size;
    EXPECT_EQ(placed_params.size, clipped_size);
    
}
