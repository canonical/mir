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
#include "mir/surfaces/surface_factory.h"
#include "mir/compositor/buffer_stream_surfaces.h"
#include "mir/surfaces/buffer_stream_factory.h"
#include "src/server/compositor/buffer_bundle.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/renderables.h"
#include "mir/geometry/rectangle.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/graphics/renderer.h"
#include "mir/surfaces/surface.h"
#include "mir/input/input_channel_factory.h"
#include "mir/input/input_channel.h"
#include "mir_test_doubles/mock_renderable.h"
#include "mir_test_doubles/mock_surface_renderer.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/stub_input_registrar.h"
#include "mir_test_doubles/stub_input_channel.h"
#include "mir_test_doubles/mock_input_registrar.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_buffer_stream.h"
#include "mir_test_doubles/mock_surface_info.h"
#include "mir_test_doubles/mock_input_info.h"
#include "mir_test_doubles/mock_graphics_info.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"

#include <memory>
#include <stdexcept>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace ms = mir::surfaces;
namespace msh = mir::shell;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

class NullBufferBundle : public mc::BufferBundle
{
public:
    virtual std::shared_ptr<mc::Buffer> client_acquire() { return std::shared_ptr<mc::Buffer>(); }
    virtual void client_release(std::shared_ptr<mc::Buffer> const&) {}
    virtual std::shared_ptr<mc::Buffer> compositor_acquire(){ return std::shared_ptr<mc::Buffer>(); };
    virtual void compositor_release(std::shared_ptr<mc::Buffer> const&){}
    virtual void force_client_abort() {}
    void force_requests_to_complete() {}
    virtual void allow_framedropping(bool) {}
    virtual mc::BufferProperties properties() const { return mc::BufferProperties{}; };
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
    MOCK_METHOD1(renderable_operator, void(mg::Renderable&));
};

struct StubOperatorForRenderables : public mc::OperatorForRenderables
{
    void operator()(mg::Renderable&)
    {
    }
};

struct StubInputChannelFactory : public mi::InputChannelFactory
{
    std::shared_ptr<mi::InputChannel> make_input_channel()
    {
        return std::make_shared<mtd::StubInputChannel>();
    }
};

struct StubInputChannel : public mi::InputChannel
{
    StubInputChannel(int server_fd, int client_fd)
        : s_fd(server_fd),
          c_fd(client_fd)
    {
    }
    
    int client_fd() const
    {
        return c_fd;
    }
    int server_fd() const
    {
        return s_fd;
    }

    int const s_fd;    
    int const c_fd;
};

struct MockSurfaceAllocator : public ms::SurfaceFactory
{
    MOCK_METHOD2(create_surface, std::shared_ptr<ms::Surface>(msh::SurfaceCreationParameters const&,
                                                              std::function<void()> const&)); 
};

static ms::DepthId const default_depth{0};

struct SurfaceStack : public ::testing::Test
{
    void SetUp()
    {
        using namespace testing;
        default_params = msh::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}});

        auto info = std::make_shared<mtd::MockSurfaceInfo>();
        auto input_info = std::make_shared<mtd::MockInputInfo>();
        auto gfx_info = std::make_shared<mtd::MockGraphicsInfo>();
        auto buffer_stream = std::make_shared<mtd::StubBufferStream>();

        //TODO: this should be a real Stub from mtd, once ms::Surface is an interface
        stub_surface1 = std::make_shared<ms::Surface>(info, gfx_info, buffer_stream,
            input_info, std::shared_ptr<mir::input::InputChannel>(), []{});
        stub_surface2 = std::make_shared<ms::Surface>(info, gfx_info, buffer_stream,
            input_info, std::shared_ptr<mir::input::InputChannel>(), []{});
        stub_surface3 = std::make_shared<ms::Surface>(info, gfx_info, buffer_stream,
            input_info, std::shared_ptr<mir::input::InputChannel>(), []{});

        ON_CALL(mock_surface_allocator, create_surface(_,_))
            .WillByDefault(Return(stub_surface1));
    }

    mtd::StubInputRegistrar input_registrar;
    MockSurfaceAllocator mock_surface_allocator;
    msh::SurfaceCreationParameters default_params;
    std::shared_ptr<ms::Surface> stub_surface1;
    std::shared_ptr<ms::Surface> stub_surface2;
    std::shared_ptr<ms::Surface> stub_surface3;
};

}

TEST_F(SurfaceStack, surface_creation_creates_surface_and_owns)
{
    using namespace testing;

    EXPECT_CALL(mock_surface_allocator, create_surface(_,_))
        .Times(1)
        .WillOnce(Return(stub_surface1));

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(input_registrar));

    auto use_count = stub_surface1.use_count();

    auto surface = stack.create_surface(default_params, default_depth);
        {
            EXPECT_EQ(stub_surface1, surface.lock());
        }
    EXPECT_LT(use_count, stub_surface1.use_count());

    stack.destroy_surface(surface);

    EXPECT_EQ(use_count, stub_surface1.use_count());
}

TEST_F(SurfaceStack, view_applies_renderer_to_all_surfaces_in_view)
{
    using namespace ::testing;

    EXPECT_CALL(mock_surface_allocator, create_surface(_,_))
        .WillOnce(Return(stub_surface1))
        .WillOnce(Return(stub_surface2))
        .WillOnce(Return(stub_surface3));

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(input_registrar));
    stack.create_surface(default_params, default_depth);
    stack.create_surface(default_params, default_depth);
    stack.create_surface(default_params, default_depth);

    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator;

    Sequence seq1, seq2;
    EXPECT_CALL(filter, filter(info1))
        .InSequence(seq1)
        .WillOnce(Return(true));
    EXPECT_CALL(filter, filter(info2))
        .InSequence(seq1)
        .WillOnce(Return(false));
    EXPECT_CALL(filter, filter(info3))
        .InSequence(seq1)
        .WillOnce(Return(true));

    EXPECT_CALL(renderable_operator, renderable_operator(info1, texture1))
        .InSequence(seq2);
    EXPECT_CALL(renderablne_operator, renderable_operator(info3, texture3))
        .InSequence(seq2);

    stack.for_each_if(filter, renderable_operator);
}

#if 0
/* FIXME: This test doesn't do what it says! */
TEST(
    SurfaceStack,
    scenegraph_query_locks_the_stack)
{
    using namespace ::testing;

    MockBufferStreamFactory buffer_stream_factory;
    StubInputChannelFactory input_factory;
    mtd::StubInputRegistrar input_registrar;
    mtd::MockSurfaceRenderer renderer;
    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator(&renderer);

    EXPECT_CALL(filter, filter(_)).Times(0);
    EXPECT_CALL(renderable_operator, renderable_operator(_)).Times(0);
    EXPECT_CALL(
        buffer_stream_factory,
        create_buffer_stream(_)).Times(0);
    EXPECT_CALL(renderer, render(_,_)).Times(0);

    ms::SurfaceStack stack(mt::fake_shared(buffer_stream_factory), 
        mt::fake_shared(input_factory), mt::fake_shared(input_registrar));

    {
        stack.for_each_if(filter, renderable_operator);
    }
}


TEST(SurfaceStack, test_stacking_order)
{
    using namespace ::testing;

    MockBufferStreamFactory buffer_stream_factory;
    StubInputChannelFactory input_factory;
    mtd::StubInputRegistrar input_registrar;

    EXPECT_CALL(
        buffer_stream_factory,
        create_buffer_stream(_)).Times(AtLeast(1));

    ms::SurfaceStack stack(mt::fake_shared(buffer_stream_factory), 
        mt::fake_shared(input_factory), mt::fake_shared(input_registrar));

    auto surface1 = stack.create_surface(
        msh::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}), default_depth);
    auto surface2 = stack.create_surface(
        msh::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}), default_depth);
    auto surface3 = stack.create_surface(
        msh::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}), default_depth);

    mtd::MockSurfaceRenderer renderer;
    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator(&renderer);

    ON_CALL(filter, filter(_)).WillByDefault(Return(true));
    EXPECT_CALL(renderer, render(_,_)).Times(3);
    EXPECT_CALL(filter, filter(_)).Times(3);

    {
      InSequence seq;

      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface1.lock()))).Times(1);
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface2.lock()))).Times(1);
      EXPECT_CALL(renderable_operator, renderable_operator(Ref(*surface3.lock()))).Times(1);
    }

    stack.for_each_if(filter, renderable_operator);
}

namespace
{

struct StubBufferStreamFactory : public ms::BufferStreamFactory
{
    std::shared_ptr<ms::BufferStream> create_buffer_stream(mc::BufferProperties const&)
    {
        return std::make_shared<mc::BufferStreamSurfaces>(
            std::make_shared<NullBufferBundle>());
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

    ms::SurfaceStack stack{std::make_shared<StubBufferStreamFactory>(),
        std::make_shared<StubInputChannelFactory>(),
            std::make_shared<mtd::StubInputRegistrar>()};
    stack.set_change_callback(std::bind(&MockCallback::call, &mock_cb));

    std::weak_ptr<ms::Surface> surface = stack.create_surface(
        msh::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}), default_depth);
}

TEST(SurfaceStack, destroy_surface_notifies_changes)
{
    using namespace ::testing;

    MockCallback mock_cb;

    EXPECT_CALL(mock_cb, call()).Times(1);

    ms::SurfaceStack stack{std::make_shared<StubBufferStreamFactory>(),
        std::make_shared<StubInputChannelFactory>(),
            std::make_shared<mtd::StubInputRegistrar>()};
    stack.set_change_callback(std::bind(&MockCallback::call, &mock_cb));

    std::weak_ptr<ms::Surface> surface = stack.create_surface(
        msh::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}), default_depth);

    Mock::VerifyAndClearExpectations(&mock_cb);
    EXPECT_CALL(mock_cb, call()).Times(1);

    stack.destroy_surface(surface);
}

TEST(SurfaceStack, surfaces_are_emitted_by_layer)
{
    using namespace ::testing;

    ms::SurfaceStack stack{std::make_shared<StubBufferStreamFactory>(),
        std::make_shared<StubInputChannelFactory>(),
            std::make_shared<mtd::StubInputRegistrar>()};
    mtd::MockSurfaceRenderer renderer;
    MockFilterForRenderables filter;
    MockOperatorForRenderables renderable_operator(&renderer);

    auto surface1 = stack.create_surface(msh::a_surface(), ms::DepthId{0});
    auto surface2 = stack.create_surface(msh::a_surface(), ms::DepthId{1});
    auto surface3 = stack.create_surface(msh::a_surface(), ms::DepthId{0});

    {
        InSequence seq;

        EXPECT_CALL(filter, filter(Ref(*surface1.lock()))).Times(1)
            .WillOnce(Return(false));
        EXPECT_CALL(filter, filter(Ref(*surface3.lock()))).Times(1)
            .WillOnce(Return(false));
        EXPECT_CALL(filter, filter(Ref(*surface2.lock()))).Times(1)
            .WillOnce(Return(false));
    }

    stack.for_each_if(filter, renderable_operator);
}

TEST(SurfaceStack, input_registrar_is_notified_of_surfaces)
{
    using namespace ::testing;

    mtd::MockInputRegistrar registrar;
    Sequence seq;

    auto channel = std::make_shared<StubInputChannel>(4,3);
    MockInputChannelFactory input_factory; 
    EXPECT_CALL(input_factory, make_input_channel())
        .Times(1)
        .WillOnce(Return(channel));
    EXPECT_CALL(registrar, input_channel_opened(_,_))
        .InSequence(seq);
    EXPECT_CALL(registrar, input_channel_closed(_))
        .InSequence(seq);

    ms::SurfaceStack stack{std::make_shared<StubBufferStreamFactory>(),
            mt::fake_shared(input_factory),
            mt::fake_shared(registrar)};
    
    auto s = stack.create_surface(msh::a_surface(), default_depth);
    stack.destroy_surface(s);
}

TEST(SurfaceStack, surface_receives_fds_from_input_channel_factory)
{
    using namespace ::testing;

    int const server_input_fd = 11;
    int const client_input_fd = 7;
    
    mtd::StubInputRegistrar registrar;
    StubInputChannel input_channel(server_input_fd, client_input_fd);
    MockInputChannelFactory input_factory;
    
    EXPECT_CALL(input_factory, make_input_channel())
        .Times(1).WillOnce(Return(mt::fake_shared(input_channel)));
    ms::SurfaceStack stack{std::make_shared<StubBufferStreamFactory>(),
        mt::fake_shared(input_factory),
            std::make_shared<mtd::StubInputRegistrar>()};

    auto surface = stack.create_surface(msh::a_surface(), default_depth).lock();
    EXPECT_EQ(client_input_fd, surface->client_input_fd());
}

TEST(SurfaceStack, raise_to_top_alters_render_ordering)
{
    using namespace ::testing;

    ms::SurfaceStack stack{std::make_shared<StubBufferStreamFactory>(),
        std::make_shared<StubInputChannelFactory>(),
            std::make_shared<mtd::StubInputRegistrar>()};

    MockFilterForRenderables filter;
    StubOperatorForRenderables stub_operator;
    
    auto surface1 = stack.create_surface(msh::a_surface(), default_depth);
    auto surface2 = stack.create_surface(msh::a_surface(), default_depth);
    auto surface3 = stack.create_surface(msh::a_surface(), default_depth);
    
    ON_CALL(filter, filter(_)).WillByDefault(Return(false));

    {
        InSequence seq;

        // After surface creation.
        EXPECT_CALL(filter, filter(Ref(*surface1.lock()))).Times(1);
        EXPECT_CALL(filter, filter(Ref(*surface2.lock()))).Times(1);
        EXPECT_CALL(filter, filter(Ref(*surface3.lock()))).Times(1);
        // After raising surface1
        EXPECT_CALL(filter, filter(Ref(*surface2.lock()))).Times(1);
        EXPECT_CALL(filter, filter(Ref(*surface3.lock()))).Times(1);
        EXPECT_CALL(filter, filter(Ref(*surface1.lock()))).Times(1);
    }
    stack.for_each_if(filter, stub_operator);
    stack.raise(surface1.lock());
    stack.for_each_if(filter, stub_operator);
}

TEST(SurfaceStack, depth_id_trumps_raise)
{
    using namespace ::testing;

    ms::SurfaceStack stack{std::make_shared<StubBufferStreamFactory>(),
        std::make_shared<StubInputChannelFactory>(),
            std::make_shared<mtd::StubInputRegistrar>()};

    MockFilterForRenderables filter;
    StubOperatorForRenderables stub_operator;
    
    auto surface1 = stack.create_surface(msh::a_surface(), ms::DepthId{0});
    auto surface2 = stack.create_surface(msh::a_surface(), ms::DepthId{0});
    auto surface3 = stack.create_surface(msh::a_surface(), ms::DepthId{1});
    
    ON_CALL(filter, filter(_)).WillByDefault(Return(false));

    {
        InSequence seq;

        // After surface creation.
        EXPECT_CALL(filter, filter(Ref(*surface1.lock()))).Times(1);
        EXPECT_CALL(filter, filter(Ref(*surface2.lock()))).Times(1);
        EXPECT_CALL(filter, filter(Ref(*surface3.lock()))).Times(1);
        // After raising surface1
        EXPECT_CALL(filter, filter(Ref(*surface2.lock()))).Times(1);
        EXPECT_CALL(filter, filter(Ref(*surface1.lock()))).Times(1);
        EXPECT_CALL(filter, filter(Ref(*surface3.lock()))).Times(1);
    }
    stack.for_each_if(filter, stub_operator);
    stack.raise(surface1);
    stack.for_each_if(filter, stub_operator);
}

TEST(SurfaceStack, raise_throw_behavior)
{
    using namespace ::testing;

    ms::SurfaceStack stack{std::make_shared<StubBufferStreamFactory>(),
        std::make_shared<StubInputChannelFactory>(),
            std::make_shared<mtd::StubInputRegistrar>()};
    
    std::shared_ptr<ms::Surface> null_surface{nullptr};

    EXPECT_THROW({
            stack.raise(null_surface);
    }, std::runtime_error);
}
#endif
