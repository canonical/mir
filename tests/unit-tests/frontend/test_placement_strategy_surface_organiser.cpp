/*
 * Copyright © 2012 Canonical Ltd.
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

#include <mir_test_doubles/null_buffer_bundle.h>
#include <mir_test_doubles/mock_surface_organiser.h>

#include <mir/frontend/placement_strategy_surface_organiser.h>
#include <mir/frontend/placement_strategy.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{

struct MockPlacementStrategy : public mf::PlacementStrategy
{
    MOCK_METHOD2(place, void(ms::SurfaceCreationParameters const&, ms::SurfaceCreationParameters&));
};

struct PlacementStrategySurfaceOrganiserSetup : public testing::Test
{
    void SetUp()
    {
        using namespace ::testing;
        
        dummy_surface = std::make_shared<ms::Surface>(ms::a_surface().name,
                                                      std::make_shared<mtd::NullBufferBundle>());

        underlying_surface_organiser = std::make_shared<mtd::MockSurfaceOrganiser>();
        ON_CALL(*underlying_surface_organiser, create_surface(_)).WillByDefault(Return(dummy_surface));

        placement_strategy = std::make_shared<MockPlacementStrategy>();
    }
    std::shared_ptr<mtd::MockSurfaceOrganiser> underlying_surface_organiser;
    std::shared_ptr<ms::Surface> dummy_surface;
    std::shared_ptr<MockPlacementStrategy> placement_strategy;
};

struct SurfaceParameterUpdater
{
    SurfaceParameterUpdater(ms::SurfaceCreationParameters const& parameters)
        : parameters(parameters)
    {
    }
    void update_parameters(ms::SurfaceCreationParameters const& /* unusued_input_parameters */,
                           ms::SurfaceCreationParameters &output_parameters)
    {
        output_parameters = parameters;
    }
    
    ms::SurfaceCreationParameters parameters;
};

} // namespace 

TEST_F(PlacementStrategySurfaceOrganiserSetup, forwards_calls_to_underlying_organiser)
{
    using namespace ::testing;

    {
        InSequence seq;
        EXPECT_CALL(*underlying_surface_organiser, create_surface(_)).Times(1);
        EXPECT_CALL(*underlying_surface_organiser, hide_surface(_)).Times(1);
        EXPECT_CALL(*underlying_surface_organiser, show_surface(_)).Times(1);
        EXPECT_CALL(*underlying_surface_organiser, destroy_surface(_)).Times(1);
    }
    
    EXPECT_CALL(*placement_strategy, place(_,_)).Times(1);

    mf::PlacementStrategySurfaceOrganiser organiser(underlying_surface_organiser, placement_strategy);
    auto params = ms::a_surface();
    
    auto surface = organiser.create_surface(params);
    organiser.hide_surface(surface);
    organiser.show_surface(surface);
    organiser.destroy_surface(surface);
}

TEST_F(PlacementStrategySurfaceOrganiserSetup, offers_create_surface_parameters_to_placement_strategy)
{
    using namespace ::testing;

    mf::PlacementStrategySurfaceOrganiser organiser(underlying_surface_organiser, placement_strategy);
    
    EXPECT_CALL(*underlying_surface_organiser, create_surface(_)).Times(1);
    
    auto params = ms::a_surface();
    EXPECT_CALL(*placement_strategy, place(Ref(params), _)).Times(1);
    
    organiser.create_surface(params);
}

TEST_F(PlacementStrategySurfaceOrganiserSetup, forwards_create_surface_parameters_from_placement_strategy_to_underlying_organiser)
{
    using namespace ::testing;

    mf::PlacementStrategySurfaceOrganiser organiser(underlying_surface_organiser, placement_strategy);
    
    auto params = ms::a_surface();

    auto placed_params = params;
    placed_params.size.width = geom::Width{100};
    SurfaceParameterUpdater param_updater(placed_params);
    
    EXPECT_CALL(*placement_strategy, place(Ref(params), _)).Times(1)
        .WillOnce(Invoke(&param_updater, &SurfaceParameterUpdater::update_parameters));
    EXPECT_CALL(*underlying_surface_organiser, create_surface(placed_params));
    
    organiser.create_surface(params);
}



