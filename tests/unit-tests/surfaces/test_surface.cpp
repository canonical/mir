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
#include "mir_test/mock_buffer.h"

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
        pf = geom::PixelFormat::rgba_8888;
        size = geom::Size{geom::Width{43}, geom::Height{420}};
        stride = geom::Stride{4 * size.width.as_uint32_t()};
        mock_buffer_bundle = std::make_shared<testing::NiceMock<mc::MockBufferBundle>>();
    }

    std::string surface_name;
    std::shared_ptr<testing::NiceMock<mc::MockBufferBundle>> mock_buffer_bundle;
    geom::PixelFormat pf;
    geom::Stride stride;
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

TEST_F(SurfaceCreation, test_surface_advance_buffer)
{
    using namespace testing;
    ms::Surface surf(surface_name, mock_buffer_bundle );
    auto graphics_resource = std::make_shared<mc::GraphicBufferClientResource>();

    EXPECT_CALL(*mock_buffer_bundle, secure_client_buffer())
        .Times(1)
        .WillOnce(Return(graphics_resource));

    surf.advance_client_buffer();
}

TEST_F(SurfaceCreation, test_surface_gets_ipc_from_bundle)
{
    using namespace testing;

    mc::BufferId{4} id;
    auto ipc_package = std::make_shared<mc::BufferIPCPackage>();
    auto mock_buffer = std::make_shared<mc::MockBuffer>();

    ms::Surface surf(surface_name, mock_buffer_bundle );
    auto graphics_resource = std::make_shared<mc::GraphicBufferClientResource>(ipc_package, mock_buffer, id);
    EXPECT_CALL(*mock_buffer_bundle, secure_client_buffer())
        .Times(AtLeast(0))
        .WillOnce(Return(graphics_resource));
    surf.advance_client_buffer();

    auto ret_ipc = surf.get_buffer_ipc_package();
    EXPECT_EQ(ret_ipc.get(), graphics_resource->ipc_package.get()); 
}

TEST_F(SurfaceCreation, test_surface_gets_id_from_bundle)
{
    using namespace testing;

    mc::BufferId{4} id;
    auto ipc_package = std::make_shared<mc::BufferIPCPackage>();
    auto mock_buffer = std::make_shared<mc::MockBuffer>();

    ms::Surface surf(surface_name, mock_buffer_bundle );
    auto graphics_resource = std::make_shared<mc::GraphicBufferClientResource>(ipc_package, mock_buffer, id);
    EXPECT_CALL(*mock_buffer_bundle, secure_client_buffer())
        .Times(AtLeast(0))
        .WillOnce(Return(graphics_resource));
    surf.advance_client_buffer();

    auto ret_id = surf.get_buffer_id();
    EXPECT_EQ(ret_id, set_id); 
}

TEST_F(SurfaceCreation, test_surface_gets_top_left)
{
    using namespace testing;

    ms::Surface surf{surface_name, mock_buffer_bundle};

    auto ret_top_left = surf.top_left();

    EXPECT_EQ(geom::Point(), ret_top_left); 
}

TEST_F(SurfaceCreation, test_surface_move_to)
{
    using namespace testing;

    ms::Surface surf{surface_name, mock_buffer_bundle};

    geom::Point p{geom::X{55}, geom::Y{66}};

    surf.move_to(p);

    auto ret_top_left = surf.top_left();

    EXPECT_EQ(p, ret_top_left); 
}

TEST_F(SurfaceCreation, test_surface_gets_identity_transformation)
{
    using namespace testing;

    ms::Surface surf{surface_name, mock_buffer_bundle};

    auto ret_transformation = surf.transformation();

    EXPECT_EQ(glm::mat4(), ret_transformation); 
}

TEST_F(SurfaceCreation, test_surface_set_rotation)
{
    using namespace testing;

    ms::Surface surf{surface_name, mock_buffer_bundle};
    surf.set_rotation(60.0f, glm::vec3{0.0f, 0.0f, 1.0f});

    auto ret_transformation = surf.transformation();

    EXPECT_NE(glm::mat4(), ret_transformation); 
}

TEST_F(SurfaceCreation, test_surface_texture_locks_back_buffer_from_bundle)
{
    using namespace testing;

    ms::Surface surf{surface_name, mock_buffer_bundle};
    auto buffer = std::make_shared<NiceMock<mc::MockBuffer>>(size, stride, pf);

    EXPECT_CALL(*mock_buffer_bundle, lock_back_buffer())
        .Times(1)
        .WillOnce(Return(buffer));

    auto ret_texture = surf.texture();

    EXPECT_EQ(buffer.get(), ret_texture.get()); 
}

TEST_F(SurfaceCreation, test_surface_gets_opaque_alpha)
{
    using namespace testing;

    ms::Surface surf{surface_name, mock_buffer_bundle};

    auto ret_alpha = surf.alpha();

    EXPECT_EQ(1.0f, ret_alpha); 
}

TEST_F(SurfaceCreation, test_surface_set_alpha)
{
    using namespace testing;

    ms::Surface surf{surface_name, mock_buffer_bundle};
    float alpha = 0.67f;

    surf.set_alpha(0.67);
    auto ret_alpha = surf.alpha();

    EXPECT_EQ(alpha, ret_alpha); 
}
