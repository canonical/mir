/*
 * Copyright © 2012-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#include "src/server/scene/application_session.h"
#include "mir/events/event_private.h"
#include "mir/graphics/buffer.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/surface_factory.h"
#include "mir/scene/null_session_listener.h"
#include "mir/client_visible_error.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_surface_stack.h"
#include "mir/test/doubles/mock_surface.h"
#include "mir/test/doubles/mock_session_listener.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/stub_surface_factory.h"
#include "mir/test/doubles/stub_buffer_stream_factory.h"
#include "mir/test/doubles/stub_buffer_stream.h"
#include "mir/test/doubles/null_snapshot_strategy.h"
#include "mir/test/doubles/null_event_sink.h"
#include "mir/test/doubles/mock_event_sink.h"
#include "mir/test/doubles/null_prompt_session.h"
#include "mir/test/doubles/stub_display_configuration.h"
#include "mir/test/doubles/stub_buffer_allocator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
static std::shared_ptr<mtd::MockSurface> make_mock_surface()
{
    using namespace testing;
    auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();
    ON_CALL(*surface, size()).WillByDefault(Return(geom::Size { 100, 100 }));
    return surface;
}

struct MockBufferStreamFactory : public ms::BufferStreamFactory
{
    MOCK_METHOD2(create_buffer_stream, std::shared_ptr<mc::BufferStream>(
        mf::BufferStreamId, mg::BufferProperties const&));
    MOCK_METHOD3(create_buffer_stream, std::shared_ptr<mc::BufferStream>(
        mf::BufferStreamId, int, mg::BufferProperties const&));
};


class MockSnapshotStrategy : public ms::SnapshotStrategy
{
public:
    ~MockSnapshotStrategy() noexcept {}

    MOCK_METHOD2(take_snapshot_of,
                void(std::shared_ptr<mc::BufferStream> const&,
                     ms::SnapshotCallback const&));
};

struct MockSnapshotCallback
{
    void operator()(ms::Snapshot const& snapshot)
    {
        operator_call(snapshot);
    }
    MOCK_METHOD1(operator_call, void(ms::Snapshot const&));
};

MATCHER(IsNullSnapshot, "")
{
    return arg.size == mir::geometry::Size{} &&
           arg.stride == mir::geometry::Stride{} &&
           arg.pixels == nullptr;
}

MATCHER_P(EqPromptSessionEventState, state, "") {
  return arg.type() == mir_event_type_prompt_session_state_change && arg.to_prompt_session()->new_state() == state;
}

MATCHER_P(HasParent, parent, "")
{
    return arg.parent.lock() == parent;
}

MATCHER(IsMirWindowOutputEvent, "")
{
    return mir_event_get_type(&arg) == mir_event_type_window_output;
}

struct StubSurfaceStack : public msh::SurfaceStack
{
    void raise(std::weak_ptr<ms::Surface> const&) override
    {
    }
    void raise(SurfaceSet const&) override
    {
    }
    void add_surface(std::shared_ptr<ms::Surface> const&, mi::InputReceptionMode) override
    {
    }
    void remove_surface(std::weak_ptr<ms::Surface> const&) override
    {
    }
    auto surface_at(mir::geometry::Point) const -> std::shared_ptr<ms::Surface> override
    {
        return std::shared_ptr<ms::Surface>{};
    }
    void add_observer(std::shared_ptr<ms::Observer> const&) override
    {
    }
    void remove_observer(std::weak_ptr<ms::Observer> const&) override
    {
    }
};

struct ApplicationSession : public testing::Test
{
    ApplicationSession()
        : event_sink(std::make_shared<mtd::NullEventSink>()),
          stub_session_listener(std::make_shared<ms::NullSessionListener>()),
          stub_surface_stack(std::make_shared<StubSurfaceStack>()),
          null_snapshot_strategy(std::make_shared<mtd::NullSnapshotStrategy>()),
          pid(0),
          name("test-session-name")
    {
    }

    std::shared_ptr<ms::ApplicationSession> make_application_session_with_stubs()
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_stack, stub_surface_factory, stub_buffer_stream_factory,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           mtd::StubDisplayConfig{},
           event_sink, allocator);
    }
    
    std::shared_ptr<ms::ApplicationSession> make_application_session(
        std::shared_ptr<ms::BufferStreamFactory> const& bstream_factory,
        std::shared_ptr<ms::SurfaceFactory> const& surface_factory)
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_stack, surface_factory, bstream_factory,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           mtd::StubDisplayConfig{},
           event_sink, allocator);
    }

    std::shared_ptr<ms::ApplicationSession> make_application_session(
        std::shared_ptr<msh::SurfaceStack> const& surface_stack,
        std::shared_ptr<ms::SurfaceFactory> const& surface_factory)
    {
        return std::make_shared<ms::ApplicationSession>(
           surface_stack, surface_factory, stub_buffer_stream_factory,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           mtd::StubDisplayConfig{},
           event_sink, allocator);
    }
    std::shared_ptr<ms::ApplicationSession> make_application_session_with_coordinator(
        std::shared_ptr<msh::SurfaceStack> const& surface_stack)
    {
        return std::make_shared<ms::ApplicationSession>(
           surface_stack, stub_surface_factory, stub_buffer_stream_factory,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           mtd::StubDisplayConfig{},
           event_sink, allocator);
    }
    
    std::shared_ptr<ms::ApplicationSession> make_application_session_with_listener(
        std::shared_ptr<ms::SessionListener> const& session_listener)
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_stack, stub_surface_factory, stub_buffer_stream_factory,
           pid, name,
           null_snapshot_strategy,
           session_listener,
           mtd::StubDisplayConfig{},
           event_sink, allocator);
    }


    std::shared_ptr<ms::ApplicationSession> make_application_session_with_buffer_stream_factory(
        std::shared_ptr<ms::BufferStreamFactory> const& buffer_stream_factory)
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_stack, stub_surface_factory, buffer_stream_factory,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           mtd::StubDisplayConfig{},
           event_sink, allocator);
    }

    std::shared_ptr<mtd::NullEventSink> const event_sink;
    std::shared_ptr<ms::NullSessionListener> const stub_session_listener;
    std::shared_ptr<StubSurfaceStack> const stub_surface_stack;
    std::shared_ptr<ms::SnapshotStrategy> const null_snapshot_strategy;
    std::shared_ptr<mtd::StubBufferStreamFactory> const stub_buffer_stream_factory =
        std::make_shared<mtd::StubBufferStreamFactory>();
    std::shared_ptr<mtd::StubSurfaceFactory> const stub_surface_factory{std::make_shared<mtd::StubSurfaceFactory>()};
    std::shared_ptr<mtd::StubBufferStream> const stub_buffer_stream{std::make_shared<mtd::StubBufferStream>()};
    std::shared_ptr<mtd::StubBufferAllocator> const allocator{
        std::make_shared<mtd::StubBufferAllocator>()};
    pid_t pid;
    std::string name;
    mg::BufferProperties properties { geom::Size{1,1}, mir_pixel_format_abgr_8888, mg::BufferUsage::hardware };
};

struct MockSurfaceFactory : ms::SurfaceFactory
{
    MOCK_METHOD2(create_surface, std::shared_ptr<ms::Surface>(
        std::list<ms::StreamInfo> const&, ms::SurfaceCreationParameters const& params));
};
}

TEST_F(ApplicationSession, adds_created_surface_to_coordinator)
{
    using namespace ::testing;

    NiceMock<MockSurfaceFactory> mock_surface_factory;
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    std::shared_ptr<ms::Surface> mock_surface = make_mock_surface();

    EXPECT_CALL(mock_surface_factory, create_surface(_,_))
        .WillOnce(Return(mock_surface));
    EXPECT_CALL(surface_stack, add_surface(mock_surface,_));
    auto session = make_application_session(
        mt::fake_shared(surface_stack), mt::fake_shared(mock_surface_factory));

    ms::SurfaceCreationParameters params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto surf = session->create_surface(params, event_sink);

    session->destroy_surface(surf);
}

TEST_F(ApplicationSession, attempt_to_destroy_non_existent_stream_throws)
{
    using namespace ::testing;
    NiceMock<MockSurfaceFactory> mock_surface_factory;
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    auto session = make_application_session(
        mt::fake_shared(surface_stack), mt::fake_shared(mock_surface_factory));

    mf::BufferStreamId made_up_id{332};
    EXPECT_THROW({
        session->destroy_buffer_stream(made_up_id);
    }, std::runtime_error);
}

TEST_F(ApplicationSession, can_destroy_buffer_stream_after_destroying_surface)
{
    using namespace ::testing;

    NiceMock<MockSurfaceFactory> mock_surface_factory;
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    std::shared_ptr<ms::Surface> mock_surface = make_mock_surface();

    EXPECT_CALL(mock_surface_factory, create_surface(_,_))
        .WillOnce(Return(mock_surface));
    EXPECT_CALL(surface_stack, add_surface(mock_surface,_));
    auto session = make_application_session(
        mt::fake_shared(surface_stack), mt::fake_shared(mock_surface_factory));

    auto buffer_stream = session->create_buffer_stream(properties);
    ms::SurfaceCreationParameters params = ms::a_surface()
        .with_buffer_stream(buffer_stream);
    auto surf = session->create_surface(params, event_sink);

    session->destroy_surface(surf);
    session->destroy_buffer_stream(buffer_stream);
}

TEST_F(ApplicationSession, notifies_listener_of_create_and_destroy_surface)
{
    using namespace ::testing;

    mtd::MockSessionListener listener;
    EXPECT_CALL(listener, surface_created(_, _))
        .Times(1);
    EXPECT_CALL(listener, destroying_surface(_, _))
        .Times(1);

    auto session = make_application_session_with_listener(mt::fake_shared(listener));

    ms::SurfaceCreationParameters params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto surf = session->create_surface(params, event_sink);

    session->destroy_surface(surf);
}

TEST_F(ApplicationSession, notifies_listener_of_surface_destruction_via_session_destruction)
{
    using namespace ::testing;

    auto mock_surface = make_mock_surface();

    mtd::MockSessionListener listener;
    EXPECT_CALL(listener, surface_created(_, _)).Times(1);
    EXPECT_CALL(listener, destroying_surface(_, _)).Times(1);

    {
        auto session = make_application_session_with_listener(mt::fake_shared(listener));

        ms::SurfaceCreationParameters params = ms::a_surface()
            .with_buffer_stream(session->create_buffer_stream(properties));
        session->create_surface(params, event_sink);
    }
}

TEST_F(ApplicationSession, throws_on_get_invalid_surface)
{
    using namespace ::testing;

    auto app_session = make_application_session_with_stubs();

    mf::SurfaceId invalid_surface_id(1);

    EXPECT_THROW({
            app_session->get_surface(invalid_surface_id);
    }, std::runtime_error);
}

TEST_F(ApplicationSession, throws_on_destroy_invalid_surface)
{
    using namespace ::testing;

    auto app_session = make_application_session_with_stubs();

    mf::SurfaceId invalid_surface_id(1);

    EXPECT_THROW({
            app_session->destroy_surface(invalid_surface_id);
    }, std::runtime_error);
}

TEST_F(ApplicationSession, default_surface_is_first_surface)
{
    using namespace ::testing;

    auto app_session = make_application_session_with_stubs();

    ms::SurfaceCreationParameters params = ms::a_surface()
        .with_buffer_stream(app_session->create_buffer_stream(properties));
    auto id1 = app_session->create_surface(params, nullptr);
    auto id2 = app_session->create_surface(params, nullptr);
    auto id3 = app_session->create_surface(params, nullptr);

    auto default_surf = app_session->default_surface();
    EXPECT_EQ(app_session->get_surface(id1), default_surf);
    app_session->destroy_surface(id1);

    default_surf = app_session->default_surface();
    EXPECT_EQ(app_session->get_surface(id2), default_surf);
    app_session->destroy_surface(id2);

    default_surf = app_session->default_surface();
    EXPECT_EQ(app_session->get_surface(id3), default_surf);
    app_session->destroy_surface(id3);
}

TEST_F(ApplicationSession, foreign_surface_has_no_successor)
{
    auto session1 = make_application_session_with_stubs();
    ms::SurfaceCreationParameters params = ms::a_surface()
        .with_buffer_stream(session1->create_buffer_stream(properties));
    auto id1 = session1->create_surface(params, nullptr);
    auto surf1 = session1->surface(id1);
    auto id2 = session1->create_surface(params, nullptr);

    auto session2 = make_application_session_with_stubs();

    EXPECT_THROW({session2->surface_after(surf1);},
                 std::runtime_error);

    session1->destroy_surface(id1);
    session1->destroy_surface(id2);
}

TEST_F(ApplicationSession, surface_after_one_is_self)
{
    auto session = make_application_session_with_stubs();
    ms::SurfaceCreationParameters params = ms::a_surface()
        .with_buffer_stream(session->create_buffer_stream(properties));
    auto id = session->create_surface(params, nullptr);
    auto surf = session->surface(id);

    EXPECT_EQ(surf, session->surface_after(surf));

    session->destroy_surface(id);
}

TEST_F(ApplicationSession, surface_after_cycles_through_all)
{
    auto app_session = make_application_session_with_stubs();

    ms::SurfaceCreationParameters params = ms::a_surface()
        .with_buffer_stream(app_session->create_buffer_stream(properties));

    int const N = 3;
    std::shared_ptr<ms::Surface> surf[N];
    mf::SurfaceId id[N];

    for (int i = 0; i < N; ++i)
    {
        id[i] = app_session->create_surface(params, nullptr);
        surf[i] = app_session->surface(id[i]);

        if (i > 0)
        {
            ASSERT_NE(surf[i], surf[i-1]);
        }
    }

    for (int i = 0; i < N-1; ++i)
        ASSERT_EQ(surf[i+1], app_session->surface_after(surf[i]));

    EXPECT_EQ(surf[0], app_session->surface_after(surf[N-1]));

    for (int i = 0; i < N; ++i)
        app_session->destroy_surface(id[i]);
}

TEST_F(ApplicationSession, session_visbility_propagates_to_surfaces)
{
    using namespace ::testing;

    auto mock_surface = make_mock_surface();

    NiceMock<MockSurfaceFactory> surface_factory;
    ON_CALL(surface_factory, create_surface(_,_)).WillByDefault(Return(mock_surface));
    NiceMock<mtd::MockSurfaceStack> surface_stack;
    auto app_session = make_application_session(mt::fake_shared(surface_stack), mt::fake_shared(surface_factory));

    {
        InSequence seq;
        EXPECT_CALL(*mock_surface, hide()).Times(1);
        EXPECT_CALL(*mock_surface, show()).Times(1);
    }

    ms::SurfaceCreationParameters params = ms::a_surface()
        .with_buffer_stream(app_session->create_buffer_stream(properties));
    auto surf = app_session->create_surface(params, event_sink);

    app_session->hide();
    app_session->show();

    app_session->destroy_surface(surf);
}

TEST_F(ApplicationSession, takes_snapshot_of_default_surface)
{
    using namespace ::testing;

    auto mock_surface = make_mock_surface();
    NiceMock<MockSurfaceFactory> surface_factory;
    MockBufferStreamFactory mock_buffer_stream_factory;
    std::shared_ptr<mc::BufferStream> const mock_stream = std::make_shared<mtd::MockBufferStream>();
    ON_CALL(mock_buffer_stream_factory, create_buffer_stream(_,_)).WillByDefault(Return(mock_stream));
    ON_CALL(surface_factory, create_surface(_,_)).WillByDefault(Return(mock_surface));
    NiceMock<mtd::MockSurfaceStack> surface_stack;

    auto const snapshot_strategy = std::make_shared<MockSnapshotStrategy>();

    EXPECT_CALL(*snapshot_strategy, take_snapshot_of(mock_stream, _));

    ms::ApplicationSession app_session(
        mt::fake_shared(surface_stack),
        mt::fake_shared(surface_factory),
        mt::fake_shared(mock_buffer_stream_factory),
        pid, name,
        snapshot_strategy,
        std::make_shared<ms::NullSessionListener>(),
        mtd::StubDisplayConfig{},
        event_sink, allocator);

    ms::SurfaceCreationParameters params = ms::a_surface()
        .with_buffer_stream(app_session.create_buffer_stream(properties));
    auto surface = app_session.create_surface(params, event_sink);
    app_session.take_snapshot(ms::SnapshotCallback());
    app_session.destroy_surface(surface);
}

TEST_F(ApplicationSession, returns_null_snapshot_if_no_default_surface)
{
    using namespace ::testing;

    auto snapshot_strategy = std::make_shared<MockSnapshotStrategy>();
    MockSnapshotCallback mock_snapshot_callback;

    ms::ApplicationSession app_session(
        stub_surface_stack, stub_surface_factory,
        stub_buffer_stream_factory,
        pid, name,
        snapshot_strategy,
        std::make_shared<ms::NullSessionListener>(),
        mtd::StubDisplayConfig{},
        event_sink, allocator);

    EXPECT_CALL(*snapshot_strategy, take_snapshot_of(_,_)).Times(0);
    EXPECT_CALL(mock_snapshot_callback, operator_call(IsNullSnapshot()));

    app_session.take_snapshot(std::ref(mock_snapshot_callback));
}

TEST_F(ApplicationSession, process_id)
{
    using namespace ::testing;

    pid_t const session_pid{__LINE__};

    ms::ApplicationSession app_session(
        stub_surface_stack, stub_surface_factory,
        stub_buffer_stream_factory,
        session_pid, name,
        null_snapshot_strategy,
        std::make_shared<ms::NullSessionListener>(),
        mtd::StubDisplayConfig{},
        event_sink, allocator);

    EXPECT_THAT(app_session.process_id(), Eq(session_pid));
}

TEST_F(ApplicationSession, can_destroy_surface_bstream)
{
    auto session = make_application_session_with_stubs();
    mf::BufferStreamId stream_id = session->create_buffer_stream(properties);
    ms::SurfaceCreationParameters params = ms::a_surface()
        .with_buffer_stream(stream_id);
    auto id = session->create_surface(params, event_sink);
    session->destroy_buffer_stream(stream_id);
    EXPECT_THROW({
        session->get_buffer_stream(stream_id);
    }, std::runtime_error);
    session->destroy_surface(id);
}

MATCHER(StreamEq, "")
{
    return (std::get<0>(arg).stream == std::get<1>(arg).stream) &&
        (std::get<0>(arg).displacement == std::get<1>(arg).displacement);
}

TEST_F(ApplicationSession, sets_and_looks_up_surface_streams)
{
    using namespace testing;
    NiceMock<MockBufferStreamFactory> mock_bufferstream_factory;
    NiceMock<MockSurfaceFactory> mock_surface_factory;

    auto mock_surface = make_mock_surface();
    EXPECT_CALL(mock_surface_factory, create_surface(_,_))
        .WillOnce(Return(mock_surface));
    
    std::array<std::shared_ptr<mc::BufferStream>,3> streams {{
        std::make_shared<mtd::StubBufferStream>(),
        std::make_shared<mtd::StubBufferStream>(),
        std::make_shared<mtd::StubBufferStream>()
    }};
    EXPECT_CALL(mock_bufferstream_factory, create_buffer_stream(_,_))
        .WillOnce(Return(streams[0]))
        .WillOnce(Return(streams[1]))
        .WillOnce(Return(streams[2]));

    auto stream_properties = mg::BufferProperties{{8,8}, mir_pixel_format_argb_8888, mg::BufferUsage::hardware};
    auto session = make_application_session(
        mt::fake_shared(mock_bufferstream_factory),
        mt::fake_shared(mock_surface_factory));

    auto stream_id0 = session->create_buffer_stream(stream_properties);
    auto stream_id1 = session->create_buffer_stream(stream_properties);
    auto stream_id2 = session->create_buffer_stream(stream_properties);

    std::list<ms::StreamInfo> info {
        {streams[2], geom::Displacement{0,3}, {}},
        {streams[0], geom::Displacement{-1,1}, {}},
        {streams[1], geom::Displacement{0,2}, {}}
    };

    session->create_surface(
        ms::a_surface().with_buffer_stream(stream_id0), event_sink);

    EXPECT_CALL(*mock_surface, set_streams(Pointwise(StreamEq(), info)));
    session->configure_streams(*mock_surface, {
        {stream_id2, geom::Displacement{0,3}, {}},
        {stream_id0, geom::Displacement{-1,1}, {}},
        {stream_id1, geom::Displacement{0,2}, {}}
    });
}

TEST_F(ApplicationSession, buffer_stream_constructed_with_requested_parameters)
{
    using namespace ::testing;
    
    geom::Size const buffer_size{geom::Width{1}, geom::Height{1}};

    mtd::StubBufferStream stream;
    MockBufferStreamFactory factory;

    mg::BufferProperties properties(buffer_size, mir_pixel_format_argb_8888, mg::BufferUsage::software);
    
    EXPECT_CALL(factory, create_buffer_stream(_,properties)).Times(1)
        .WillOnce(Return(mt::fake_shared(stream)));

    auto session = make_application_session_with_buffer_stream_factory(mt::fake_shared(factory));
    auto id = session->create_buffer_stream(properties);

    EXPECT_TRUE(session->get_buffer_stream(id) != nullptr);
    
    session->destroy_buffer_stream(id);
    
    EXPECT_THROW({
            session->get_buffer_stream(id);
    }, std::runtime_error);
}

TEST_F(ApplicationSession, buffer_stream_constructed_with_swapinterval_1)
{
    using namespace ::testing;
    
    geom::Size const buffer_size{geom::Width{1}, geom::Height{1}};

    mtd::MockBufferStream stream;
    MockBufferStreamFactory factory;

    mg::BufferProperties properties(buffer_size, mir_pixel_format_argb_8888, mg::BufferUsage::software);

    EXPECT_CALL(stream, allow_framedropping(true))
        .Times(0); 
    EXPECT_CALL(factory, create_buffer_stream(_,properties)).Times(1)
        .WillOnce(Return(mt::fake_shared(stream)));

    auto session = make_application_session_with_buffer_stream_factory(mt::fake_shared(factory));
    auto id = session->create_buffer_stream(properties);
    session->destroy_buffer_stream(id);
}

MATCHER_P(HasSingleStream, value, "")
{
    using namespace testing;
    EXPECT_THAT(arg.size(), Eq(1));
    if (arg.size() < 1 ) return false;
    EXPECT_THAT(arg.front().stream.get(), Eq(value.get())); 
    return !(::testing::Test::HasFailure());
}

TEST_F(ApplicationSession, surface_uses_prexisting_buffer_stream_if_set)
{
    using namespace testing;

    mtd::StubBufferStreamFactory bufferstream_factory;
    NiceMock<MockSurfaceFactory> mock_surface_factory;

    geom::Size const buffer_size{geom::Width{1}, geom::Height{1}};

    mg::BufferProperties properties(buffer_size, mir_pixel_format_argb_8888, mg::BufferUsage::software);

    auto session = make_application_session(
        mt::fake_shared(bufferstream_factory),
        mt::fake_shared(mock_surface_factory));

    auto id = session->create_buffer_stream(properties);
    auto stream = session->get_buffer_stream(id);

    EXPECT_CALL(mock_surface_factory, create_surface(HasSingleStream(stream),_))
        .WillOnce(Return(make_mock_surface()));

    ms::SurfaceCreationParameters params = ms::SurfaceCreationParameters{}
        .of_name("Aardavks")
        .of_type(mir_window_type_normal)
        .with_buffer_stream(id);

    session->create_surface(params, event_sink);
}

namespace
{
struct ApplicationSessionSender : public ApplicationSession
{
    ApplicationSessionSender() :
        app_session(
            stub_surface_stack,
            stub_surface_factory,
            stub_buffer_stream_factory,
            pid,
            name,
            null_snapshot_strategy,
            stub_session_listener,
            mtd::StubDisplayConfig{},
            mt::fake_shared(sender),
            allocator)
    {
    }

    testing::NiceMock<mtd::MockEventSink> sender;
    ms::ApplicationSession app_session;
};
}

TEST_F(ApplicationSessionSender, display_config_sender)
{
    using namespace ::testing;

    mtd::StubDisplayConfig stub_config;
    EXPECT_CALL(sender, handle_display_config_change(testing::Ref(stub_config)))
        .Times(1);

    app_session.send_display_config(stub_config);
}

TEST_F(ApplicationSessionSender, error_sender)
{
    using namespace ::testing;

    class TestError : public mir::ClientVisibleError
    {
    public:
        TestError()
            : ClientVisibleError("Hello")
        {
        }

        MirErrorDomain domain() const noexcept override
        {
            return static_cast<MirErrorDomain>(42);
        }

        uint32_t code() const noexcept override
        {
            return 8u;
        }
    } error;
    EXPECT_CALL(sender, handle_error(Ref(error)))
        .Times(1);

    app_session.send_error(error);
}

TEST_F(ApplicationSessionSender, lifecycle_event_sender)
{
    using namespace ::testing;

    MirLifecycleState exp_state = mir_lifecycle_state_will_suspend;

    EXPECT_CALL(sender, handle_lifecycle_event(exp_state)).Times(1);
    app_session.set_lifecycle_state(mir_lifecycle_state_will_suspend);
}

TEST_F(ApplicationSessionSender, start_prompt_session)
{
    using namespace ::testing;

    EXPECT_CALL(sender, handle_event(EqPromptSessionEventState(mir_prompt_session_state_started))).Times(1);
    app_session.start_prompt_session();
}

TEST_F(ApplicationSessionSender, stop_prompt_session)
{
    using namespace ::testing;

    EXPECT_CALL(sender, handle_event(EqPromptSessionEventState(mir_prompt_session_state_stopped))).Times(1);
    app_session.stop_prompt_session();
}

namespace
{
class ObserverPreservingSurface : public mtd::MockSurface
{
public:
    void add_observer(std::shared_ptr<mir::scene::SurfaceObserver> const &observer) override
    {
        return BasicSurface::add_observer(observer);
    }

    void remove_observer(std::weak_ptr<mir::scene::SurfaceObserver> const &observer) override
    {
        return BasicSurface::remove_observer(observer);
    }
};

class ObserverPreservingSurfaceFactory : public ms::SurfaceFactory
{
public:
    std::shared_ptr<ms::Surface> create_surface(
        std::list<ms::StreamInfo> const&,
        mir::scene::SurfaceCreationParameters const& params) override
    {
        using namespace testing;
        auto mock = std::make_shared<NiceMock<ObserverPreservingSurface>>();
        ON_CALL(*mock, size()).WillByDefault(Return(params.size));
        return mock;
    };
};

int calculate_dpi(geom::Size const& resolution, geom::Size const& size)
{
    float constexpr mm_per_inch = 25.4f;

    auto diagonal_mm = sqrt(size.height.as_int()*size.height.as_int()
                            + size.width.as_int()*size.width.as_int());
    auto diagonal_px = sqrt(resolution.height.as_int() * resolution.height.as_int()
                            + resolution.width.as_int() * resolution.width.as_int());

    return diagonal_px / diagonal_mm * mm_per_inch;
}

struct ApplicationSessionSurfaceOutput : public ApplicationSession
{
    ApplicationSessionSurfaceOutput() :
        high_dpi(static_cast<mg::DisplayConfigurationOutputId>(5), {3840, 2160}, {509, 286}, 2.5f, 60.0, mir_form_factor_monitor),
        projector(static_cast<mg::DisplayConfigurationOutputId>(2), {1280, 1024}, {800, 600}, 0.5f, 50.0, mir_form_factor_projector),
        stub_surface_factory{std::make_shared<ObserverPreservingSurfaceFactory>()},
        sender{std::make_shared<testing::NiceMock<mtd::MockEventSink>>()},
        app_session(
            stub_surface_stack,
            stub_surface_factory,
            stub_buffer_stream_factory,
            pid,
            name,
            null_snapshot_strategy,
            stub_session_listener,
            mtd::StubDisplayConfig{},
            sender, allocator)
    {
    }

    struct TestOutput
    {
        TestOutput(
            mg::DisplayConfigurationOutputId id,
            geom::Size const& resolution,
            geom::Size const& physical_size,
            float scale,
            double hz,
            MirFormFactor form_factor) :
            output{id, resolution, physical_size, mir_pixel_format_argb_8888, hz, true},
            form_factor{form_factor},
            scale{scale},
            dpi{calculate_dpi(resolution, physical_size)},
            width{resolution.width.as_int()},
            id{static_cast<uint32_t>(output.id.as_value())}
        {
            output.scale = scale;
            output.form_factor = form_factor;
        }

        mtd::StubDisplayConfigurationOutput output;
        MirFormFactor form_factor;
        float scale;
        int dpi;
        int width;
        uint32_t id;
    };

    TestOutput const high_dpi;
    TestOutput const projector;
    std::shared_ptr<ms::SurfaceFactory> const stub_surface_factory;
    std::shared_ptr<testing::NiceMock<mtd::MockEventSink>> sender;
    ms::ApplicationSession app_session;
};
}

namespace
{
MATCHER_P(SurfaceOutputEventFor, output, "")
{
    using namespace testing;

    if (mir_event_get_type(arg) != mir_event_type_window_output)
    {
        *result_listener << "Event is not a MirWindowOutputEvent";
        return 0;
    }

    auto const event = mir_event_get_window_output_event(arg);

    if (output.output.current_mode_index >= output.output.modes.size())
        return false;

    auto const& mode = output.output.modes[output.output.current_mode_index];

    return
        ExplainMatchResult(
            Eq(output.dpi),
            mir_window_output_event_get_dpi(event),
            result_listener) &&
        ExplainMatchResult(
            Eq(output.form_factor),
            mir_window_output_event_get_form_factor(event),
            result_listener) &&
        ExplainMatchResult(
            Eq(output.scale),
            mir_window_output_event_get_scale(event),
            result_listener) &&
        ExplainMatchResult(
            Eq(mode.vrefresh_hz),
            mir_window_output_event_get_refresh_rate(event),
            result_listener) &&
        ExplainMatchResult(
            Eq(output.id),
            mir_window_output_event_get_output_id(event),
            result_listener);
}
}

TEST_F(ApplicationSessionSurfaceOutput, sends_surface_output_events_to_surfaces)
{
    using namespace ::testing;

    MirWindowOutputEvent event;
    bool event_received{false};

    EXPECT_CALL(*sender, handle_event(IsMirWindowOutputEvent()))
        .WillOnce(Invoke([&event, &event_received](MirEvent const& ev)
                         {
                             if (event.type() == mir_event_type_window_output)
                             {
                                 event = *ev.to_window_output();
                             }
                             event_received = true;
                         }));

    std::vector<mg::DisplayConfigurationOutput> outputs =
        {
            high_dpi.output
        };
    mtd::StubDisplayConfig config(outputs);
    app_session.send_display_config(config);

    ms::SurfaceCreationParameters params = ms::SurfaceCreationParameters{}
        .of_size({100, 100})
        .with_buffer_stream(app_session.create_buffer_stream(properties));
    auto surf_id = app_session.create_surface(params, sender);
    auto surface = app_session.surface(surf_id);

    ASSERT_TRUE(event_received);
    EXPECT_THAT(&event, SurfaceOutputEventFor(high_dpi));
}

TEST_F(ApplicationSessionSurfaceOutput, sends_correct_surface_details_to_surface)
{
    using namespace ::testing;

    std::array<TestOutput const*, 2> outputs{{ &high_dpi, &projector }};

    std::array<MirWindowOutputEvent, 2> event;
    int events_received{0};

    ON_CALL(*sender, handle_event(IsMirWindowOutputEvent()))
        .WillByDefault(Invoke([&event, &events_received](MirEvent const& ev)
                         {
                             if (ev.type() == mir_event_type_window_output)
                             {
                                 event[events_received] = *ev.to_window_output();
                             }
                             ++events_received;
                         }));

    ms::SurfaceCreationParameters params = ms::SurfaceCreationParameters{}
        .of_size({100, 100})
        .with_buffer_stream(app_session.create_buffer_stream(properties));

    mf::SurfaceId ids[2];
    std::shared_ptr<ms::Surface> surfaces[2];

    ids[0] = app_session.create_surface(params, sender);
    ids[1] = app_session.create_surface(params, sender);

    surfaces[0] = app_session.surface(ids[0]);
    surfaces[1] = app_session.surface(ids[1]);

    surfaces[0]->move_to({0 + 100, 100});
    surfaces[1]->move_to({outputs[0]->width + 100, 100});

    // Reset events recieved; we may have received events from the move.
    events_received = 0;

    std::vector<mg::DisplayConfigurationOutput> configuration_outputs =
        {
            outputs[0]->output, outputs[1]->output
        };
    configuration_outputs[0].top_left = {0, 0};
    configuration_outputs[1].top_left = {outputs[0]->width, 0};

    mtd::StubDisplayConfig config(configuration_outputs);
    app_session.send_display_config(config);

    ASSERT_THAT(events_received, Eq(2));

    for (int i = 0; i < 2 ; ++i)
    {
        EXPECT_THAT(event[i].to_window_output()->surface_id(), Eq(ids[i].as_value()));
        EXPECT_THAT(&event[i], SurfaceOutputEventFor(*outputs[i]));
    }
}

TEST_F(ApplicationSessionSurfaceOutput, sends_details_of_the_hightest_scale_factor_display_on_overlap)
{
    using namespace ::testing;

    std::array<TestOutput const*, 2> outputs{{ &projector, &high_dpi }};

    MirWindowOutputEvent event;
    bool event_received{false};

    ON_CALL(*sender, handle_event(IsMirWindowOutputEvent()))
        .WillByDefault(Invoke([&event, &event_received](MirEvent const& ev)
                              {
                                  if (ev.type() == mir_event_type_window_output)
                                  {
                                      event = *ev.to_window_output();
                                  }
                                  event_received = true;
                              }));

    ms::SurfaceCreationParameters params = ms::SurfaceCreationParameters{}
        .of_size({100, 100})
        .with_buffer_stream(app_session.create_buffer_stream(properties));

    auto id = app_session.create_surface(params, sender);
    auto surface = app_session.surface(id);

    // This should overlap both outputs
    surface->move_to({outputs[0]->width - 50, 100});

    std::vector<mg::DisplayConfigurationOutput> configuration_outputs =
        {
            outputs[0]->output, outputs[1]->output
        };

    // Put the higher-scale output on the right, so a surface's top_left coordinate
    // can be in the lower-scale output but overlap with the higher-scale output.
    configuration_outputs[0].top_left = {0, 0};
    configuration_outputs[1].top_left = {outputs[0]->width, 0};

    mtd::StubDisplayConfig config(configuration_outputs);
    app_session.send_display_config(config);

    ASSERT_TRUE(event_received);

    EXPECT_THAT(event.to_window_output()->surface_id(), Eq(id.as_value()));
    EXPECT_THAT(&event, SurfaceOutputEventFor(high_dpi));
}

TEST_F(ApplicationSessionSurfaceOutput, surfaces_on_edges_get_correct_values)
{
    using namespace ::testing;

    std::array<TestOutput const*, 2> outputs{{ &projector, &high_dpi }};

    MirWindowOutputEvent event;
    bool event_received{false};

    ON_CALL(*sender, handle_event(IsMirWindowOutputEvent()))
        .WillByDefault(Invoke([&event, &event_received](MirEvent const& ev)
                              {
                                  if (ev.type() == mir_event_type_window_output)
                                  {
                                      event = *ev.to_window_output();
                                  }
                                  event_received = true;
                              }));

    std::vector<mg::DisplayConfigurationOutput> configuration_outputs =
        {
            outputs[0]->output, outputs[1]->output
        };

    // Put the higher-scale output on the right, so a surface's top_left coordinate
    // can be in the lower-scale output but overlap with the higher-scale output.
    configuration_outputs[0].top_left = {0, 0};
    configuration_outputs[1].top_left = {outputs[0]->width, 0};

    mtd::StubDisplayConfig config(configuration_outputs);
    app_session.send_display_config(config);

    ms::SurfaceCreationParameters params = ms::SurfaceCreationParameters{}
        .of_size({640, 480})
        .with_buffer_stream(app_session.create_buffer_stream(properties));

    auto id = app_session.create_surface(params, sender);
    auto surface = app_session.surface(id);

    // This should solidly overlap both outputs
    surface->move_to({outputs[0]->width - ((surface->size().width.as_uint32_t()) / 2), 100});

    ASSERT_TRUE(event_received);
    EXPECT_THAT(&event, SurfaceOutputEventFor(high_dpi));

    event_received = false;
    // This should be *just* entirely on the projector
    surface->move_to({outputs[0]->width - surface->size().width.as_uint32_t(), 100});

    ASSERT_TRUE(event_received);
    EXPECT_THAT(&event, SurfaceOutputEventFor(projector));

    event_received = false;
    // This should have a single pixel overlap on the high_dpi
    surface->move_to({outputs[0]->width - (surface->size().width.as_uint32_t() - 1), 100});

    ASSERT_TRUE(event_received);
    EXPECT_THAT(&event, SurfaceOutputEventFor(high_dpi));
}

TEST_F(ApplicationSessionSurfaceOutput, sends_surface_output_event_on_move)
{
    using namespace ::testing;

    std::array<TestOutput const*, 2> outputs {{ &projector, &high_dpi }};

    MirWindowOutputEvent event;
    int events_received{0};


    ON_CALL(*sender, handle_event(IsMirWindowOutputEvent()))
        .WillByDefault(Invoke([&event, &events_received](MirEvent const& ev)
                              {
                                  if (ev.type() == mir_event_type_window_output)
                                  {
                                      event = *ev.to_window_output();
                                  }
                                  events_received++;
                              }));

    std::vector<mg::DisplayConfigurationOutput> configuration_outputs =
        {
            outputs[0]->output, outputs[1]->output
        };

    // Put the higher-scale output on the right, so a surface's top_left coordinate
    // can be in the lower-scale output but overlap with the higher-scale output.
    configuration_outputs[0].top_left = {0, 0};
    configuration_outputs[1].top_left = {outputs[0]->width, 0};

    mtd::StubDisplayConfig config(configuration_outputs);
    app_session.send_display_config(config);

    ms::SurfaceCreationParameters params = ms::SurfaceCreationParameters{}
        .of_size({100, 100})
        .with_buffer_stream(app_session.create_buffer_stream(properties));

    auto id = app_session.create_surface(params, sender);
    auto surface = app_session.surface(id);

    // This should overlap both outputs
    surface->move_to({outputs[0]->width - 50, 100});


    ASSERT_THAT(events_received, Ge(1));
    auto events_expected = events_received + 1;

    EXPECT_THAT(event.to_window_output()->surface_id(), Eq(id.as_value()));
    EXPECT_THAT(&event, SurfaceOutputEventFor(high_dpi));

    // Now solely on the left output
    surface->move_to({0, 0});

    ASSERT_THAT(events_received, Eq(events_expected));
    events_expected++;

    EXPECT_THAT(event.to_window_output()->surface_id(), Eq(id.as_value()));
    EXPECT_THAT(&event, SurfaceOutputEventFor(projector));

    // Now solely on the right output
    surface->move_to({outputs[0]->width + 100, 100});

    ASSERT_THAT(events_received, Eq(events_expected));
    events_expected++;

    EXPECT_THAT(event.to_window_output()->surface_id(), Eq(id.as_value()));
    EXPECT_THAT(&event, SurfaceOutputEventFor(high_dpi));
}

TEST_F(ApplicationSessionSurfaceOutput, sends_surface_output_event_on_move_only_if_changed)
{
    using namespace ::testing;

    std::array<TestOutput const*, 2> outputs {{ &projector, &high_dpi }};

    int events_received{0};

    ON_CALL(*sender, handle_event(IsMirWindowOutputEvent()))
        .WillByDefault(Invoke([&events_received](MirEvent const&)
                              {
                                  events_received++;
                              }));

    std::vector<mg::DisplayConfigurationOutput> configuration_outputs =
        {
            outputs[0]->output, outputs[1]->output
        };

    // Put the higher-scale output on the right, so a surface's top_left coordinate
    // can be in the lower-scale output but overlap with the higher-scale output.
    configuration_outputs[0].top_left = {0, 0};
    configuration_outputs[1].top_left = {outputs[0]->width, 0};

    mtd::StubDisplayConfig config(configuration_outputs);
    app_session.send_display_config(config);

    ms::SurfaceCreationParameters params = ms::SurfaceCreationParameters{}
        .of_size({100, 100})
        .with_buffer_stream(app_session.create_buffer_stream(properties));

    auto id = app_session.create_surface(params, sender);
    auto surface = app_session.surface(id);

    // We get an event on surface creation.
    EXPECT_THAT(events_received, Eq(1));

    // This should move within the same output, so not generate any events
    surface->move_to({outputs[0]->width - (surface->size().width.as_int() + 1), 100});

    EXPECT_THAT(events_received, Eq(1));

    // This should move to *exactly* touching the edge of the output; still no events
    surface->move_to({outputs[0]->width - surface->size().width.as_int(), 100});
    EXPECT_THAT(events_received, Eq(1));

    // This should move to just overlaping the second output; should generate an event
    surface->move_to({outputs[0]->width - (surface->size().width.as_int() - 1), 100});
    EXPECT_THAT(events_received, Eq(2));

    // Back to exactly on the original display; another event.
    surface->move_to({outputs[0]->width - surface->size().width.as_int(), 100});
    EXPECT_THAT(events_received, Eq(3));
}
