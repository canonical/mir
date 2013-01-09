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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "src/surfaces/proxy_surface.h"
#include "mir/surfaces/surface.h"
#include "mir/sessions/surface_creation_parameters.h"
#include "mir/surfaces/surface_stack_model.h"

#include "mir_test_doubles/mock_buffer_bundle.h"
#include "mir_test_doubles/mock_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::surfaces;
namespace msess = mir::sessions;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{
class MockSurfaceStackModel : public ms::SurfaceStackModel
{
public:
    MockSurfaceStackModel()
    {
        using namespace testing;

        ON_CALL(*this, create_surface(_)).
            WillByDefault(Invoke(this, &MockSurfaceStackModel::do_create_surface));

        ON_CALL(*this, destroy_surface(_)).
            WillByDefault(Invoke(this, &MockSurfaceStackModel::do_destroy_surface));
    }

    MOCK_METHOD1(create_surface, std::weak_ptr<ms::Surface> (const msess::SurfaceCreationParameters&));

    MOCK_METHOD1(destroy_surface, void (std::weak_ptr<ms::Surface> const&));

private:
    std::weak_ptr<ms::Surface> do_create_surface(const msess::SurfaceCreationParameters& params)
    {
        surface = std::make_shared<ms::Surface>(
            params.name,
            std::make_shared<testing::NiceMock<mtd::MockBufferBundle>>());
        return surface;
    }

    void do_destroy_surface(std::weak_ptr<ms::Surface> const&)
    {
        surface.reset();
    }

    std::shared_ptr<ms::Surface> surface;
};

}

TEST(SurfaceProxy, creation_and_destruction)
{
    MockSurfaceStackModel surface_stack;
    msess::SurfaceCreationParameters params;

    using namespace testing;
    InSequence sequence;
    EXPECT_CALL(surface_stack, create_surface(_)).Times(1);
    EXPECT_CALL(surface_stack, destroy_surface(_)).Times(1);

    ms::ProxySurface test(&surface_stack, params);
}

TEST(SurfaceProxy, destroy)
{
    using namespace testing;

    NiceMock<MockSurfaceStackModel> surface_stack;
    msess::SurfaceCreationParameters params;

    ms::ProxySurface test(&surface_stack, params);

    EXPECT_CALL(surface_stack, destroy_surface(_)).Times(1);

    test.destroy();

    Mock::VerifyAndClearExpectations(&surface_stack);
}
