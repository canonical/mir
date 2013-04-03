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
#include "mir/surfaces/buffer_bundle_factory.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/renderables.h"
#include "mir/geometry/rectangle.h"
#include "mir/frontend/surface_creation_parameters.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/graphics/renderer.h"
#include "mir/surfaces/surface.h"
#include "mir_test_doubles/mock_renderable.h"
#include "mir_test_doubles/mock_surface_renderer.h"
#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"

#include <memory>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace mf = mir::frontend;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

class NullBufferSwapper : public mc::BufferSwapper
{
public:
    virtual std::shared_ptr<mc::Buffer> client_acquire() { return std::shared_ptr<mc::Buffer>(); }
    virtual void client_release(std::shared_ptr<mc::Buffer> const&) {}
    virtual std::shared_ptr<mc::Buffer> compositor_acquire(){ return std::shared_ptr<mc::Buffer>(); };
    virtual void compositor_release(std::shared_ptr<mc::Buffer> const&){}
    virtual void shutdown() {}
};

struct MockBufferBundleFactory : public ms::BufferBundleFactory
{
    MockBufferBundleFactory()
    {
        using namespace ::testing;

        ON_CALL(
            *this,
            create_buffer_bundle(_))
                .WillByDefault(
                    Return(
                        std::shared_ptr<ms::BufferBundle>(
                                new mc::BufferBundleSurfaces(
                                std::unique_ptr<mc::BufferSwapper>(new NullBufferSwapper())))));
    }

    MOCK_METHOD1(
        create_buffer_bundle,
        std::shared_ptr<ms::BufferBundle>(mc::BufferProperties const&));
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
        renderer->render([] (std::shared_ptr<void> const&) {}, r);
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
    mc::BufferBundleSurfaces buffer_bundle(std::move(swapper_handle));
    MockBufferBundleFactory buffer_bundle_factory;

    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_))
            .Times(AtLeast(1));

    ms::SurfaceStack stack(mt::fake_shared(buffer_bundle_factory));
    std::weak_ptr<ms::Surface> surface = stack.create_surface(
        mf::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    stack.destroy_surface(surface);
}

/* FIXME: This test doesn't do what it says! */
TEST(
    SurfaceStack,
    scenegraph_query_locks_the_stack)
{
    using namespace ::testing;

    MockBufferBundleFactory buffer_bundle_factory;
    mtd::MockSurfaceRenderer renderer;
    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator(&renderer);

    EXPECT_CALL(filter, filter(_)).Times(0);
    EXPECT_CALL(renderable_operator, renderable_operator(_)).Times(0);
    EXPECT_CALL(
        buffer_bundle_factory,
        create_buffer_bundle(_)).Times(0);
    EXPECT_CALL(renderer, render(_,_)).Times(0);

    ms::SurfaceStack stack(mt::fake_shared(buffer_bundle_factory));

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

    ms::SurfaceStack stack(mt::fake_shared(buffer_bundle_factory));

    auto surface1 = stack.create_surface(
        mf::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    auto surface2 = stack.create_surface(
        mf::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    auto surface3 = stack.create_surface(
        mf::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    mtd::MockSurfaceRenderer renderer;
    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator(&renderer);

    ON_CALL(filter, filter(_)).WillByDefault(Return(true));
    ON_CALL(filter, filter(Ref(*surface3.lock()))).WillByDefault(Return(false));

    EXPECT_CALL(filter, filter(_)).Times(3);
    EXPECT_CALL(renderable_operator, renderable_operator(_)).Times(2);

    EXPECT_CALL(renderer,
                render(_,Ref(*surface1.lock()))).Times(Exactly(1));
    EXPECT_CALL(renderer,
                render(_,Ref(*surface2.lock()))).Times(Exactly(1));

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

    ms::SurfaceStack stack(mt::fake_shared(buffer_bundle_factory));

    auto surface1 = stack.create_surface(
        mf::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    auto surface2 = stack.create_surface(
        mf::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    auto surface3 = stack.create_surface(
        mf::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    mtd::MockSurfaceRenderer renderer;
    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator(&renderer);

    ON_CALL(filter, filter(_)).WillByDefault(Return(true));
    EXPECT_CALL(renderer, render(_,_)).Times(3);
    EXPECT_CALL(filter, filter(_)).Times(3);

    {
      InSequence seq;
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface3.lock()))).Times(1);
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface2.lock()))).Times(1);
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface1.lock()))).Times(1);
    }

    stack.for_each_if(filter, renderable_operator);
}

TEST(SurfaceStack, created_buffer_bundle_uses_requested_surface_parameters)
{
    using namespace ::testing;

    std::unique_ptr<mc::BufferSwapper> swapper_handle;
    MockBufferBundleFactory buffer_bundle_factory;

    geom::Size const size{geom::Size{geom::Width{1024}, geom::Height{768}}};
    geom::PixelFormat const format{geom::PixelFormat::bgr_888};
    mc::BufferUsage const usage{mc::BufferUsage::software};

    EXPECT_CALL(buffer_bundle_factory,
                create_buffer_bundle(AllOf(
                    Field(&mc::BufferProperties::size, size),
                    Field(&mc::BufferProperties::format, format),
                    Field(&mc::BufferProperties::usage, usage))))
        .Times(AtLeast(1));

    ms::SurfaceStack stack(mt::fake_shared(buffer_bundle_factory));
    std::weak_ptr<ms::Surface> surface = stack.create_surface(
        mf::a_surface().of_size(size).of_buffer_usage(usage).of_pixel_format(format));

    stack.destroy_surface(surface);
}

namespace
{

struct StubBufferBundleFactory : public ms::BufferBundleFactory
{
    std::shared_ptr<ms::BufferBundle> create_buffer_bundle(mc::BufferProperties const&)
    {
        return std::make_shared<mc::BufferBundleSurfaces>(
            std::unique_ptr<mc::BufferSwapper>(new NullBufferSwapper()));
    }
};

class MockCallback
{
public:
    MOCK_METHOD0(call, void());
};

}

TEST(SurfaceStack, create_surface_notifies_changes)
{
    using namespace ::testing;

    MockCallback mock_cb;

    EXPECT_CALL(mock_cb, call()).Times(1);

    ms::SurfaceStack stack{std::make_shared<StubBufferBundleFactory>()};
    stack.set_change_callback(std::bind(&MockCallback::call, &mock_cb));

    std::weak_ptr<ms::Surface> surface = stack.create_surface(
        mf::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
}

TEST(SurfaceStack, destroy_surface_notifies_changes)
{
    using namespace ::testing;

    MockCallback mock_cb;

    EXPECT_CALL(mock_cb, call()).Times(1);

    ms::SurfaceStack stack{std::make_shared<StubBufferBundleFactory>()};
    stack.set_change_callback(std::bind(&MockCallback::call, &mock_cb));

    std::weak_ptr<ms::Surface> surface = stack.create_surface(
        mf::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    Mock::VerifyAndClearExpectations(&mock_cb);
    EXPECT_CALL(mock_cb, call()).Times(1);

    stack.destroy_surface(surface);
}
