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
#include "mir/graphics/buffer_properties.h"
#include "mir/compositor/scene.h"
#include "mir/geometry/rectangle.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/compositor/renderer.h"
#include "mir/compositor/compositing_criteria.h"
#include "mir/surfaces/surface.h"
#include "mir/input/input_channel_factory.h"
#include "mir/input/input_channel.h"
#include "mir_test_doubles/mock_surface_renderer.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/stub_input_registrar.h"
#include "mir_test_doubles/stub_input_channel.h"
#include "mir_test_doubles/mock_input_registrar.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_buffer_stream.h"
#include "mir_test_doubles/mock_surface_state.h"
#include "mir_test_doubles/mock_input_surface.h"
#include "mir_test_doubles/mock_compositing_criteria.h"

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
    virtual std::shared_ptr<mg::Buffer> client_acquire() { return std::shared_ptr<mg::Buffer>(); }
    virtual void client_release(std::shared_ptr<mg::Buffer> const&) {}
    virtual std::shared_ptr<mg::Buffer> compositor_acquire(){ return std::shared_ptr<mg::Buffer>(); };
    virtual void compositor_release(std::shared_ptr<mg::Buffer> const&){}
    virtual std::shared_ptr<mg::Buffer> snapshot_acquire(){ return std::shared_ptr<mg::Buffer>(); };
    virtual void snapshot_release(std::shared_ptr<mg::Buffer> const&){}
    virtual void force_client_abort() {}
    void force_requests_to_complete() {}
    virtual void allow_framedropping(bool) {}
    virtual mg::BufferProperties properties() const { return mg::BufferProperties{}; };
};


struct MockFilterForScene : public mc::FilterForScene
{
    // Can not mock operator overload so need to forward
    MOCK_METHOD1(filter, bool(mc::CompositingCriteria const&));
    bool operator()(mc::CompositingCriteria const& r)
    {
        return filter(r);
    }
};

struct StubFilterForScene : public mc::FilterForScene
{
    MOCK_METHOD1(filter, bool(mc::CompositingCriteria const&));
    bool operator()(mc::CompositingCriteria const&)
    {
        return true;
    }
};

struct MockOperatorForScene : public mc::OperatorForScene
{
    MOCK_METHOD2(renderable_operator, void(mc::CompositingCriteria const&, ms::BufferStream&));
    void operator()(mc::CompositingCriteria const& state, ms::BufferStream& stream)
    {
        renderable_operator(state, stream);
    }
};

struct StubOperatorForScene : public mc::OperatorForScene
{
    void operator()(mc::CompositingCriteria const&, ms::BufferStream&)
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

struct StubBufferStreamFactory : public ms::BufferStreamFactory
{
    std::shared_ptr<ms::BufferStream> create_buffer_stream(mg::BufferProperties const&)
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

struct SurfaceStack : public ::testing::Test
{
    void SetUp()
    {
        using namespace testing;
        default_params = msh::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}});

        //TODO: this should be a real Stub from mtd, once ms::Surface is an interface
        stub_surface1 = std::make_shared<ms::Surface>(std::make_shared<mtd::MockSurfaceState>(), std::make_shared<mtd::StubBufferStream>(), std::shared_ptr<mir::input::InputChannel>());
        stub_surface2 = std::make_shared<ms::Surface>(std::make_shared<mtd::MockSurfaceState>(), std::make_shared<mtd::StubBufferStream>(), std::shared_ptr<mir::input::InputChannel>());
        stub_surface3 = std::make_shared<ms::Surface>(std::make_shared<mtd::MockSurfaceState>(), std::make_shared<mtd::StubBufferStream>(), std::shared_ptr<mir::input::InputChannel>());

        ON_CALL(mock_surface_allocator, create_surface(_,_))
            .WillByDefault(Return(stub_surface1));
    }

    mtd::StubInputRegistrar input_registrar;
    testing::NiceMock<MockSurfaceAllocator> mock_surface_allocator;
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

    auto surface = stack.create_surface(default_params);
        {
            EXPECT_EQ(stub_surface1, surface.lock());
        }
    EXPECT_LT(use_count, stub_surface1.use_count());

    stack.destroy_surface(surface);

    EXPECT_EQ(use_count, stub_surface1.use_count());
}

TEST_F(SurfaceStack, surface_skips_surface_that_is_filtered_out)
{
    using namespace ::testing;

    EXPECT_CALL(mock_surface_allocator, create_surface(_,_))
        .WillOnce(Return(stub_surface1))
        .WillOnce(Return(stub_surface2))
        .WillOnce(Return(stub_surface3));

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(input_registrar));
    auto s1 = stack.create_surface(default_params);
    auto criteria1 = s1.lock()->compositing_criteria();
    auto stream1 = s1.lock()->buffer_stream();
    auto s2 = stack.create_surface(default_params);
    auto criteria2 = s2.lock()->compositing_criteria();
    auto stream2 = s2.lock()->buffer_stream();
    auto s3 = stack.create_surface(default_params);
    auto criteria3 = s3.lock()->compositing_criteria();
    auto stream3 = s3.lock()->buffer_stream();

    MockFilterForScene filter;
    MockOperatorForScene renderable_operator;

    Sequence seq1, seq2;
    EXPECT_CALL(filter, filter(Ref(*criteria1)))
        .InSequence(seq1)
        .WillOnce(Return(true));
    EXPECT_CALL(filter, filter(Ref(*criteria2)))
        .InSequence(seq1)
        .WillOnce(Return(false));
    EXPECT_CALL(filter, filter(Ref(*criteria3)))
        .InSequence(seq1)
        .WillOnce(Return(true));
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria1), Ref(*stream1)))
        .InSequence(seq2);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria3), Ref(*stream3)))
        .InSequence(seq2);

    stack.for_each_if(filter, renderable_operator);
}

TEST_F(SurfaceStack, stacking_order)
{
    using namespace ::testing;

    EXPECT_CALL(mock_surface_allocator, create_surface(_,_))
        .WillOnce(Return(stub_surface1))
        .WillOnce(Return(stub_surface2))
        .WillOnce(Return(stub_surface3));

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(input_registrar));

    auto s1 = stack.create_surface(default_params);
    auto criteria1 = s1.lock()->compositing_criteria();
    auto stream1 = s1.lock()->buffer_stream();
    auto s2 = stack.create_surface(default_params);
    auto criteria2 = s2.lock()->compositing_criteria();
    auto stream2 = s2.lock()->buffer_stream();
    auto s3 = stack.create_surface(default_params);
    auto criteria3 = s3.lock()->compositing_criteria();
    auto stream3 = s3.lock()->buffer_stream();

    StubFilterForScene filter;
    MockOperatorForScene renderable_operator;
    Sequence seq;
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria1), Ref(*stream1)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria2), Ref(*stream2)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria3), Ref(*stream3)))
        .InSequence(seq);

    stack.for_each_if(filter, renderable_operator);
}

TEST_F(SurfaceStack, notify_on_create_and_destroy_surface)
{
    using namespace ::testing;
    NiceMock<MockCallback> mock_cb;
    EXPECT_CALL(mock_cb, call())
        .Times(2);

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(input_registrar));
    stack.set_change_callback(std::bind(&MockCallback::call, &mock_cb));
    auto surface = stack.create_surface(default_params);
    stack.destroy_surface(surface);
}

TEST_F(SurfaceStack, surfaces_are_emitted_by_layer)
{
    using namespace ::testing;

    EXPECT_CALL(mock_surface_allocator, create_surface(_,_))
        .WillOnce(Return(stub_surface1))
        .WillOnce(Return(stub_surface2))
        .WillOnce(Return(stub_surface3));

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(input_registrar));
    auto s1 = stack.create_surface(default_params.of_depth(ms::DepthId{0}));
    auto criteria1 = s1.lock()->compositing_criteria();
    auto stream1 = s1.lock()->buffer_stream();
    auto s2 = stack.create_surface(default_params.of_depth(ms::DepthId{1}));
    auto criteria2 = s2.lock()->compositing_criteria();
    auto stream2 = s2.lock()->buffer_stream();
    auto s3 = stack.create_surface(default_params.of_depth(ms::DepthId{0}));
    auto criteria3 = s3.lock()->compositing_criteria();
    auto stream3 = s3.lock()->buffer_stream();

    StubFilterForScene filter;
    MockOperatorForScene renderable_operator;
    Sequence seq;
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria1), Ref(*stream1)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria3), Ref(*stream3)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria2), Ref(*stream2)))
        .InSequence(seq);

    stack.for_each_if(filter, renderable_operator);
}

TEST_F(SurfaceStack, input_registrar_is_notified_of_surfaces)
{
    using namespace ::testing;

    mtd::MockInputRegistrar registrar;

    Sequence seq;
    EXPECT_CALL(registrar, input_channel_opened(_,_,_))
        .InSequence(seq);
    EXPECT_CALL(registrar, input_channel_closed(_))
        .InSequence(seq);

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(registrar));
    
    auto s = stack.create_surface(msh::a_surface());
    stack.destroy_surface(s);
}

TEST_F(SurfaceStack, input_registrar_is_notified_of_input_monitor_surfaces)
{
    using namespace ::testing;

    mtd::MockInputRegistrar registrar;

    Sequence seq;
    EXPECT_CALL(registrar, input_channel_opened(_,_,mi::InputReceptionMode::receives_all_input))
        .InSequence(seq);
    EXPECT_CALL(registrar, input_channel_closed(_))
        .InSequence(seq);

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(registrar));
    
    auto s = stack.create_surface(msh::a_surface().with_input_mode(mi::InputReceptionMode::receives_all_input));
    stack.destroy_surface(s);
}

TEST_F(SurfaceStack, raise_to_top_alters_render_ordering)
{
    using namespace ::testing;

    EXPECT_CALL(mock_surface_allocator, create_surface(_,_))
        .WillOnce(Return(stub_surface1))
        .WillOnce(Return(stub_surface2))
        .WillOnce(Return(stub_surface3));

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(input_registrar));
    auto s1 = stack.create_surface(default_params);
    auto criteria1 = s1.lock()->compositing_criteria();
    auto stream1 = s1.lock()->buffer_stream();
    auto s2 = stack.create_surface(default_params);
    auto criteria2 = s2.lock()->compositing_criteria();
    auto stream2 = s2.lock()->buffer_stream();
    auto s3 = stack.create_surface(default_params);
    auto criteria3 = s3.lock()->compositing_criteria();
    auto stream3 = s3.lock()->buffer_stream();

    StubFilterForScene filter;
    MockOperatorForScene renderable_operator;
    Sequence seq;
    // After surface creation.
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria1), Ref(*stream1)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria2), Ref(*stream2)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria3), Ref(*stream3)))
        .InSequence(seq);
    // After raising surface1
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria2), Ref(*stream2)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria3), Ref(*stream3)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria1), Ref(*stream1)))
        .InSequence(seq);

    stack.for_each_if(filter, renderable_operator);
    stack.raise(s1.lock());
    stack.for_each_if(filter, renderable_operator);
}

TEST_F(SurfaceStack, depth_id_trumps_raise)
{
    using namespace ::testing;

    EXPECT_CALL(mock_surface_allocator, create_surface(_,_))
        .WillOnce(Return(stub_surface1))
        .WillOnce(Return(stub_surface2))
        .WillOnce(Return(stub_surface3));

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(input_registrar));
    auto s1 = stack.create_surface(default_params.of_depth(ms::DepthId{0}));
    auto criteria1 = s1.lock()->compositing_criteria();
    auto stream1 = s1.lock()->buffer_stream();
    auto s2 = stack.create_surface(default_params.of_depth(ms::DepthId{0}));
    auto criteria2 = s2.lock()->compositing_criteria();
    auto stream2 = s2.lock()->buffer_stream();
    auto s3 = stack.create_surface(default_params.of_depth(ms::DepthId{1}));
    auto criteria3 = s3.lock()->compositing_criteria();
    auto stream3 = s3.lock()->buffer_stream();

    StubFilterForScene filter;
    MockOperatorForScene renderable_operator;
    Sequence seq;
    // After surface creation.
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria1), Ref(*stream1)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria2), Ref(*stream2)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria3), Ref(*stream3)))
        .InSequence(seq);
    // After raising surface1
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria2), Ref(*stream2)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria1), Ref(*stream1)))
        .InSequence(seq);
    EXPECT_CALL(renderable_operator, renderable_operator(Ref(*criteria3), Ref(*stream3)))
        .InSequence(seq);

    stack.for_each_if(filter, renderable_operator);
    stack.raise(s1.lock());
    stack.for_each_if(filter, renderable_operator);
}

TEST_F(SurfaceStack, raise_throw_behavior)
{
    using namespace ::testing;

    ms::SurfaceStack stack(mt::fake_shared(mock_surface_allocator), mt::fake_shared(input_registrar));
    
    std::shared_ptr<ms::Surface> null_surface{nullptr};
    EXPECT_THROW({
            stack.raise(null_surface);
    }, std::runtime_error);
}
