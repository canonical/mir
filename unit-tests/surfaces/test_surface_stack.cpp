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

#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/buffer_bundle_factory.h"
#include "mir/geometry/rectangle.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/graphics/surface_renderer.h"
#include "mir/surfaces/surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;

namespace
{

struct MockBufferBundleFactory : public mc::BufferBundleFactory
{
    MockBufferBundleFactory()
    {
        using namespace ::testing;
        ON_CALL(
            *this,
            create_buffer_bundle(_, _, _))
                .WillByDefault(
                    Return(
                        std::shared_ptr<mc::BufferBundle>(
                            new mc::BufferBundle())));
    }
    
    MOCK_METHOD3(
        create_buffer_bundle,
        std::shared_ptr<mc::BufferBundle>(
            geom::Width width,
            geom::Height height,
            mc::PixelFormat pf));

};



struct MockSurfaceRenderer : public mg::SurfaceRenderer
{
    MOCK_METHOD1(render, void(const std::shared_ptr<ms::Surface>&));
};

}

TEST(
    SurfaceStack,
    surface_creation_destruction_creates_and_destroys_buffer_bundle)
{
    using namespace ::testing;

    MockBufferBundleFactory buffer_bundle_factory;
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_, _, _))
            .Times(AtLeast(1));
     
    ms::SurfaceStack stack(&buffer_bundle_factory);
    std::weak_ptr<ms::Surface> surface = stack.create_surface(
        ms::a_surface().of_size(geom::Width(1024), geom::Height(768)));

    EXPECT_EQ(1, stack.surface_count());
    
    stack.destroy_surface(surface);
}

TEST(
    SurfaceStack,
    lock_and_unlock_variants)
{
    using namespace ::testing;

    MockBufferBundleFactory buffer_bundle_factory;
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_, _, _)).Times(0);
     
    ms::SurfaceStack stack(&buffer_bundle_factory);
    stack.lock();
    EXPECT_FALSE(stack.try_lock());
    stack.unlock();
    EXPECT_TRUE(stack.try_lock());
}

TEST(
    SurfaceStack,
    scenegraph_query_locks_the_stack)
{
    using namespace ::testing;

    MockBufferBundleFactory buffer_bundle_factory;
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_, _, _)).Times(0);
     
    ms::SurfaceStack stack(&buffer_bundle_factory);

    {
        std::shared_ptr<ms::SurfaceCollection> surfaces_in_view = stack.get_surfaces_in(geom::Rectangle());
        EXPECT_FALSE(stack.try_lock());
    }
    EXPECT_TRUE(stack.try_lock());
 
}

TEST(
    SurfaceStack,
    view_applies_renderer_to_all_surfaces_in_view)
{
    using namespace ::testing;

    MockBufferBundleFactory buffer_bundle_factory;
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_, _, _))
            .WillRepeatedly(Return(std::shared_ptr<mc::BufferBundle>(new mc::BufferBundle())));
     
    ms::SurfaceStack stack(&buffer_bundle_factory);

    auto surface1 = stack.create_surface(
        ms::a_surface().of_size(geom::Width(1024), geom::Height(768)));
    auto surface2 = stack.create_surface(
        ms::a_surface().of_size(geom::Width(1024), geom::Height(768)));
    auto surface3 = stack.create_surface(
        ms::a_surface().of_size(geom::Width(1024), geom::Height(768)));

    EXPECT_EQ(3, stack.surface_count());

    MockSurfaceRenderer renderer;
    EXPECT_CALL(renderer, render(surface1.lock())).Times(Exactly(1));
    EXPECT_CALL(renderer, render(surface2.lock())).Times(Exactly(1));
    EXPECT_CALL(renderer, render(surface3.lock())).Times(Exactly(1));

    auto view = stack.get_surfaces_in(geom::Rectangle());

    view->invoke_for_each_surface(
        std::bind(&mg::SurfaceRenderer::render,
                  &renderer,
                  std::placeholders::_1));
}

