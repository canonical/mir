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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir/surfaces/surface.h"
#include "mir_test/mock_buffer_bundle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace ms = mir::surfaces;
namespace mc = mir::compositor;
namespace geom = mir::geometry;

TEST(surface, default_creation_parameters)
{
    using namespace geom;
    ms::SurfaceCreationParameters params;

    ASSERT_EQ(Width(0), params.size.width);
    ASSERT_EQ(Height(0), params.size.height);

    ASSERT_EQ(ms::a_surface(), params);
}

TEST(surface, builder_mutators)
{
    using namespace geom;
    Size const size{Width{1024}, Height{768}};

    auto params = ms::a_surface().of_size(size); 
    ms::SurfaceCreationParameters reference{std::string(), size};
    ASSERT_EQ(
        reference,
        params
        );
}

namespace
{
struct SurfaceCreation : public ::testing::Test
{
    virtual void SetUp()
    {
        surface_name = "test_surfaceA";
        pf = mc::PixelFormat::rgba_8888;
        size = geom::Size{geom::Width{43}, geom::Height{420}};
        mock_buffer_bundle = std::make_shared<mc::MockBufferBundle>();
    }

    std::string surface_name;
    std::shared_ptr<mc::MockBufferBundle> mock_buffer_bundle;
    mc::PixelFormat pf;
    geom::Size size;
};
}

TEST_F(SurfaceCreation, test_surface_gets_right_name)
{
    ms::Surface surf(surface_name, mock_buffer_bundle );

    auto str = surf.name();
    EXPECT_EQ(str, surface_name);

}

TEST_F(SurfaceCreation, test_surface_queries_bundle_for_pf)
{
    using namespace testing;

    ms::Surface surf(surface_name, mock_buffer_bundle );

    EXPECT_CALL(*mock_buffer_bundle, get_bundle_pixel_format())
        .Times(1)
        .WillOnce(Return(pf));

    auto ret_pf = surf.pixel_format();

    EXPECT_EQ(ret_pf, pf); 
}

TEST_F(SurfaceCreation, test_surface_queries_bundle_for_size)
{
    using namespace testing;

    ms::Surface surf(surface_name, mock_buffer_bundle );

    EXPECT_CALL(*mock_buffer_bundle, bundle_size())
        .Times(1)
        .WillOnce(Return(size));

    auto ret_size = surf.size();

    EXPECT_EQ(ret_size, size); 
}

TEST_F(SurfaceCreation, test_surface_gets_ipc_from_bundle)
{
    using namespace testing;

    ms::Surface surf(surface_name, mock_buffer_bundle );
    auto graphics_resource = std::make_shared<mc::GraphicBufferClientResource>();

    EXPECT_CALL(*mock_buffer_bundle, secure_client_buffer())
        .Times(1)
        .WillOnce(Return(graphics_resource));

    auto ret_ipc = surf.get_buffer_ipc_package();

    EXPECT_EQ(ret_ipc.get(), graphics_resource->ipc_package.get()); 

}
