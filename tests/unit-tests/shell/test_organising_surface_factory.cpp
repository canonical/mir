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

#include "src/server/shell/organising_surface_factory.h"
#include "mir/shell/placement_strategy.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/shell/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_event_source.h"
#include "mir/scene/surface_coordinator.h"

#include "mir_test_doubles/stub_shell_session.h"
#include "mir_test_doubles/null_event_sink.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{
struct MockSurfaceCoordinator : public ms::SurfaceCoordinator
{
    MOCK_METHOD2(add_surface, std::shared_ptr<ms::Surface>(
        msh::SurfaceCreationParameters const&,
        std::shared_ptr<ms::SurfaceObserver> const&));

    void remove_surface(std::weak_ptr<ms::Surface> const& /*surface*/) override {}
    void raise(std::weak_ptr<ms::Surface> const& /*surface*/)  override {}
};

struct MockPlacementStrategy : public msh::PlacementStrategy
{
    MOCK_METHOD2(place, msh::SurfaceCreationParameters(msh::Session const&, msh::SurfaceCreationParameters const&));
};

struct OrganisingSurfaceFactorySetup : public testing::Test
{
    void SetUp()
    {
        using namespace ::testing;
        ON_CALL(*surface_coordinator, add_surface(_, _)).WillByDefault(Return(null_surface));
    }
    std::shared_ptr<ms::Surface> null_surface;
    std::shared_ptr<MockSurfaceCoordinator> surface_coordinator = std::make_shared<MockSurfaceCoordinator>();
    std::shared_ptr<ms::SurfaceObserver> const observer = std::make_shared<ms::SurfaceEventSource>(mf::SurfaceId(), std::make_shared<mtd::NullEventSink>());
    std::shared_ptr<MockPlacementStrategy> placement_strategy = std::make_shared<MockPlacementStrategy>();
};

} // namespace

TEST_F(OrganisingSurfaceFactorySetup, offers_create_surface_parameters_to_placement_strategy)
{
    using namespace ::testing;

    msh::OrganisingSurfaceFactory factory(surface_coordinator, placement_strategy);

    mtd::StubShellSession session;
    EXPECT_CALL(*surface_coordinator, add_surface(_, _)).Times(1);

    auto params = msh::a_surface();
    EXPECT_CALL(*placement_strategy, place(Ref(session), Ref(params))).Times(1)
        .WillOnce(Return(msh::a_surface()));

    factory.create_surface(&session, params, observer);
}

TEST_F(OrganisingSurfaceFactorySetup, forwards_create_surface_parameters_from_placement_strategy_to_underlying_factory)
{
    using namespace ::testing;

    msh::OrganisingSurfaceFactory factory(surface_coordinator, placement_strategy);

    auto params = msh::a_surface();
    auto placed_params = params;
    placed_params.size.width = geom::Width{100};

    EXPECT_CALL(*placement_strategy, place(_, Ref(params))).Times(1)
        .WillOnce(Return(placed_params));
    EXPECT_CALL(*surface_coordinator, add_surface(placed_params, _));

    factory.create_surface(nullptr, params, observer);
}
