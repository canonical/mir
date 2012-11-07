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

#include "mir/surfaces/surface_stack.h"
#include "mir/compositor/buffer_bundle_surfaces.h"
#include "mir/compositor/buffer_bundle_factory.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/render_view.h"
#include "mir/geometry/rectangle.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/graphics/renderer.h"
#include "mir/surfaces/surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"

#include <memory>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;

namespace
{

class NullBufferSwapper : public mc::BufferSwapper
{
public:
    /* normally, it is a guarantee that dequeue_free_buffer or grab_last_posted
       never returns a nullptr. this nullbuffer swapper class does though */
    virtual mc::Buffer* client_acquire() { return 0; }

    /* once a client is done with the finished buffer, it must queue
       it. This modifies the buffer the compositor posts to the screen */
    virtual void client_release(mc::Buffer*) {}

    /* caller of grab_last_posted buffer should get no-wait access to the
        last posted buffer. However, the client will potentially stall
        until control of the buffer is returned via ungrab() */

    virtual mc::Buffer* compositor_acquire() { return 0; }

    virtual void compositor_release(mc::Buffer*) { }
};

struct MockBufferBundleFactory : public mc::BufferBundleFactory
{
    MockBufferBundleFactory()
    {
        using namespace ::testing;
        auto generator = std::make_shared<mc::BufferIDMonotonicIncreaseGenerator>();

        ON_CALL(
            *this,
            create_buffer_bundle(_))
                .WillByDefault(
                    Return(
                        std::shared_ptr<mc::BufferBundle>(
                                new mc::BufferBundleSurfaces(
                                std::unique_ptr<mc::BufferSwapper>(new NullBufferSwapper()), generator ))));
    }

    MOCK_METHOD1(
        create_buffer_bundle,
        std::shared_ptr<mc::BufferBundle>(mc::BufferProperties const&));
};

struct MockSurfaceRenderer : public mg::Renderer
{
    MOCK_METHOD1(render, void(mg::Renderable&));
};

struct MockFilterForRenderables : public mc::FilterForRenderables
{
    // Can not mock operator overload so need to forward
    MOCK_METHOD1(filter, bool(mg::Renderable&));
    bool operator()(mg::Renderable& r)
    {
        return filter(r);
    }
};

struct MockOperatorForRenderables : public mc::OperatorForRenderables
{
    MockOperatorForRenderables(mg::Renderer *renderer) :
        renderer(renderer)
    {
    }

    MOCK_METHOD1(renderable_operator, void(mg::Renderable&));
    void operator()(mg::Renderable& r)
    {
        // We just use this for expectations
        renderable_operator(r);
        renderer->render(r);
    }
    mg::Renderer* renderer;
};

}

TEST(
    SurfaceStack,
    surface_creation_destruction_creates_and_destroys_buffer_bundle)
{
    using namespace ::testing;

    std::unique_ptr<mc::BufferSwapper> swapper_handle;
    auto generator = std::make_shared<mc::BufferIDMonotonicIncreaseGenerator>();
    mc::BufferBundleSurfaces buffer_bundle(std::move(swapper_handle), generator );
    MockBufferBundleFactory buffer_bundle_factory;

    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_))
            .Times(AtLeast(1));

    ms::SurfaceStack stack(&buffer_bundle_factory);
    std::weak_ptr<ms::Surface> surface = stack.create_surface(
        ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    stack.destroy_surface(surface);
}

/* FIXME: This test doesn't do what it says! */
TEST(
    SurfaceStack,
    scenegraph_query_locks_the_stack)
{
    using namespace ::testing;

    MockBufferBundleFactory buffer_bundle_factory;
    MockSurfaceRenderer renderer;
    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator(&renderer);
    
    EXPECT_CALL(filter, filter(_)).Times(0);
    EXPECT_CALL(renderable_operator, renderable_operator(_)).Times(0);
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_)).Times(0);
    EXPECT_CALL(renderer, render(_)).Times(0);

    ms::SurfaceStack stack(&buffer_bundle_factory);

    {
        stack.for_each_if(filter, renderable_operator);
    }
}

/* FIXME: Why is this test working in terms of a renderer which is unrelated
   to the functionality of SurfaceStack? */
TEST(
    SurfaceStack,
    view_applies_renderer_to_all_surfaces_in_view)
{
    using namespace ::testing;

    MockBufferBundleFactory buffer_bundle_factory;
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_)).Times(AtLeast(1));
     
    ms::SurfaceStack stack(&buffer_bundle_factory);

    auto surface1 = stack.create_surface(
        ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    auto surface2 = stack.create_surface(
        ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    auto surface3 = stack.create_surface(
        ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    MockSurfaceRenderer renderer;
    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator(&renderer);
    
    ON_CALL(filter, filter(_)).WillByDefault(Return(true));
    ON_CALL(filter, filter(Ref(*surface3.lock()))).WillByDefault(Return(false));

    EXPECT_CALL(filter, filter(_)).Times(3);
    EXPECT_CALL(renderable_operator, renderable_operator(_)).Times(2);

    EXPECT_CALL(renderer,
                render(Ref(*surface1.lock()))).Times(Exactly(1));
    EXPECT_CALL(renderer,
                render(Ref(*surface2.lock()))).Times(Exactly(1));
    
    stack.for_each_if(filter, renderable_operator);
}

TEST(
    SurfaceStack,
    test_stacking_order)
{
    using namespace ::testing;

    MockBufferBundleFactory buffer_bundle_factory;
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_)).Times(AtLeast(1));
     
    ms::SurfaceStack stack(&buffer_bundle_factory);

    auto surface1 = stack.create_surface(
        ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    auto surface2 = stack.create_surface(
        ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    auto surface3 = stack.create_surface(
        ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    MockSurfaceRenderer renderer;
    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator(&renderer);
    
    ON_CALL(filter, filter(_)).WillByDefault(Return(true));
    EXPECT_CALL(renderer, render(_)).Times(3);
    EXPECT_CALL(filter, filter(_)).Times(3);

    {
      InSequence seq;
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface3.lock()))).Times(1);
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface2.lock()))).Times(1);
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface1.lock()))).Times(1);
    }

    stack.for_each_if(filter, renderable_operator);
}

TEST(
    SurfaceStack,
    test_restacking)
{
    using namespace ::testing;

    MockBufferBundleFactory buffer_bundle_factory;
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_)).Times(AtLeast(1));
     
    ms::SurfaceStack stack(&buffer_bundle_factory);

    auto surface1 = stack.create_surface(
        ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    auto surface2 = stack.create_surface(
        ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    auto surface3 = stack.create_surface(
        ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    MockSurfaceRenderer renderer;
    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator(&renderer);
    
    ON_CALL(filter, filter(_)).WillByDefault(Return(true));
    EXPECT_CALL(renderer, render(_)).Times(3);
    EXPECT_CALL(filter, filter(_)).Times(3);
    
    stack.raise_to_top(surface2);

    {
      InSequence seq;
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface2.lock()))).Times(1);
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface3.lock()))).Times(1);
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface1.lock()))).Times(1);
    }

    stack.for_each_if(filter, renderable_operator);

}



