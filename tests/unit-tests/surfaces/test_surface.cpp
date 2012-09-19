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
    ms::SurfaceCreationParameters params;

    ASSERT_EQ(geom::Width(0), params.width);
    ASSERT_EQ(geom::Height(0), params.height);

    ASSERT_EQ(ms::a_surface(), params);
}

TEST(surface, builder_mutators)
{
    geom::Width w{1024};
    geom::Height h{768};

    ASSERT_EQ(w, ms::a_surface().of_width(w).width);
    ASSERT_EQ(h, ms::a_surface().of_height(h).height);
    auto params = ms::a_surface().of_size(w, h); 
    ms::SurfaceCreationParameters reference{std::string(), w, h};
    ASSERT_EQ(
        reference,
        params
        );
    ASSERT_EQ(
        params,
        ms::a_surface().of_width(w).of_height(h));
}

namespace
{
struct SurfaceCreation : public ::testing::Test
{
    virtual void SetUp()
    {
        pf = mc::PixelFormat::rgba_8888;
        mock_buffer_bundle = std::make_shared<mc::MockBufferBundle>();
    }

    ms::SurfaceCreationParameters creation_params;
    std::shared_ptr<mc::MockBufferBundle> mock_buffer_bundle;
    mc::PixelFormat pf;
};
}

TEST_F(SurfaceCreation, test_surface_queries_bundle_for_pf)
{
    using namespace testing;

    ms::Surface surf(creation_params, mock_buffer_bundle );

    EXPECT_CALL(*mock_buffer_bundle, get_bundle_pixel_format())
        .Times(1)
        .WillOnce(Return(pf));

    auto ret_pf = surf.pixel_format();

    EXPECT_EQ(ret_pf, pf); 
}

