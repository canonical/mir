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

#include <stdexcept>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::surfaces;
namespace msess = mir::sessions;
namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{
typedef testing::NiceMock<mtd::MockBufferBundle> StubBufferBundle;

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
            std::make_shared<StubBufferBundle>());
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

TEST(BasicSurfaceProxy, client_buffer_resource_throw_behavior)
{
    using namespace testing;

    msess::SurfaceCreationParameters params;
    auto surface = std::make_shared<ms::Surface>(
        params.name,
        std::make_shared<StubBufferBundle>());

    ms::BasicProxySurface proxy_surface(surface);

    EXPECT_NO_THROW({
        proxy_surface.client_buffer_resource();
    });

    surface.reset();

    EXPECT_THROW({
        proxy_surface.client_buffer_resource();
    }, std::runtime_error);
}

TEST(BasicSurfaceProxy, size_throw_behavior)
{
    using namespace testing;

    msess::SurfaceCreationParameters params;
    auto mock_buffer_bundle = std::make_shared<StubBufferBundle>();
    EXPECT_CALL(*mock_buffer_bundle, bundle_size())
        .Times(1)
        .WillOnce(Return(geom::Size())); 
    auto surface = std::make_shared<ms::Surface>(params.name, mock_buffer_bundle);

    ms::BasicProxySurface proxy_surface(surface);

    EXPECT_NO_THROW({
        proxy_surface.size();
    });

    surface.reset();

    EXPECT_THROW({
        proxy_surface.size();
    }, std::runtime_error);
}

TEST(BasicSurfaceProxy, pixel_format_throw_behavior)
{
    using namespace testing;

    msess::SurfaceCreationParameters params;
    auto mock_buffer_bundle = std::make_shared<StubBufferBundle>();
    EXPECT_CALL(*mock_buffer_bundle, get_bundle_pixel_format())
        .Times(1)
        .WillOnce(Return(geom::PixelFormat::abgr_8888)); 
    auto surface = std::make_shared<ms::Surface>(params.name, mock_buffer_bundle);

    ms::BasicProxySurface proxy_surface(surface);

    EXPECT_NO_THROW({
        proxy_surface.pixel_format();
    });

    surface.reset();

    EXPECT_THROW({
        proxy_surface.pixel_format();
    }, std::runtime_error);
}
