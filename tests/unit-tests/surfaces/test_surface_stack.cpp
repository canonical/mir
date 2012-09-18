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
        ON_CALL(
            *this,
            create_buffer_bundle(_, _, _))
                .WillByDefault(
                    Return(
                        std::shared_ptr<mc::BufferBundle>(
                                new mc::BufferBundleSurfaces(
                                std::unique_ptr<mc::BufferSwapper>(new NullBufferSwapper())))));
    }

    MOCK_METHOD3(
        create_buffer_bundle,
        std::shared_ptr<mc::BufferBundle>(
            geom::Width width,
            geom::Height height,
            mc::PixelFormat pf));

};



struct MockSurfaceRenderer : public mg::Renderer,
                             public ms::SurfaceEnumerator
{
    MOCK_METHOD1(render, void(mg::Renderable&));
    void operator()(ms::Surface& s)
    {
        render(s);
    }
};

}

TEST(
    SurfaceStack,
    surface_creation_destruction_creates_and_destroys_buffer_bundle)
{
    using namespace ::testing;

    std::unique_ptr<mc::BufferSwapper> swapper_handle;
    mc::BufferBundleSurfaces buffer_bundle(std::move(swapper_handle));
    MockBufferBundleFactory buffer_bundle_factory;

    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_, _, _))
            .Times(AtLeast(1));

    ms::SurfaceStack stack(&buffer_bundle_factory);
    std::weak_ptr<ms::Surface> surface = stack.create_surface(
        ms::a_surface().of_size(geom::Width(1024), geom::Height(768)));

    stack.destroy_surface(surface);
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
    }
}

TEST(
    SurfaceStack,
    view_applies_renderer_to_all_surfaces_in_view)
{
    using namespace ::testing;

    MockBufferBundleFactory buffer_bundle_factory;
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_, _, _)).Times(AtLeast(1));
     
    ms::SurfaceStack stack(&buffer_bundle_factory);

    auto surface1 = stack.create_surface(
        ms::a_surface().of_size(geom::Width(1024), geom::Height(768)));
    auto surface2 = stack.create_surface(
        ms::a_surface().of_size(geom::Width(1024), geom::Height(768)));
    auto surface3 = stack.create_surface(
        ms::a_surface().of_size(geom::Width(1024), geom::Height(768)));

    MockSurfaceRenderer renderer;
    EXPECT_CALL(renderer,
                render(Ref(*surface1.lock()))).Times(Exactly(1));
    EXPECT_CALL(renderer,
                render(Ref(*surface2.lock()))).Times(Exactly(1));
    EXPECT_CALL(renderer,
                render(Ref(*surface3.lock()))).Times(Exactly(1));
    
    auto view = stack.get_surfaces_in(geom::Rectangle());

    view->invoke_for_each_surface(renderer);
}

