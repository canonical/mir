/*
 * Copyright © 2012-2014 Canonical Ltd.
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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#include "src/server/scene/application_session.h"
#include "mir/events/event_private.h"
#include "mir/graphics/buffer.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/surface_factory.h"
#include "mir/scene/null_session_listener.h"
#include "mir/test/fake_shared.h"
#include "mir/test/doubles/mock_surface_coordinator.h"
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mi = mir::input;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
static std::shared_ptr<mtd::MockSurface> make_mock_surface()
{
    return std::make_shared<testing::NiceMock<mtd::MockSurface> >();
}

struct MockBufferStreamFactory : public ms::BufferStreamFactory
{
    MOCK_METHOD3(create_buffer_stream, std::shared_ptr<mc::BufferStream>(
        mf::BufferStreamId, std::shared_ptr<mf::BufferSink> const&, mg::BufferProperties const&));
    MOCK_METHOD4(create_buffer_stream, std::shared_ptr<mc::BufferStream>(
        mf::BufferStreamId, std::shared_ptr<mf::BufferSink> const&, int, mg::BufferProperties const&));
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
  return arg.type == mir_event_type_prompt_session_state_change && arg.prompt_session.new_state == state;
}

MATCHER_P(HasParent, parent, "")
{
    return arg.parent.lock() == parent;
}

struct StubSurfaceCoordinator : public ms::SurfaceCoordinator
{
    void raise(std::weak_ptr<ms::Surface> const&) override
    {
    }
    void raise(SurfaceSet const&) override
    {
    }
    void add_surface(
        std::shared_ptr<ms::Surface> const&, ms::DepthId, mi::InputReceptionMode const&, ms::Session*) override
    {
    }
    void remove_surface(std::weak_ptr<ms::Surface> const&) override
    {
    }
    auto surface_at(mir::geometry::Point) const -> std::shared_ptr<ms::Surface>
    {
        return std::shared_ptr<ms::Surface>{};
    }
};

struct ApplicationSession : public testing::Test
{
    ApplicationSession()
        : event_sink(std::make_shared<mtd::NullEventSink>()),
          stub_session_listener(std::make_shared<ms::NullSessionListener>()),
          stub_surface_coordinator(std::make_shared<StubSurfaceCoordinator>()),
          null_snapshot_strategy(std::make_shared<mtd::NullSnapshotStrategy>()),
          pid(0),
          name("test-session-name")
    {
    }

    std::shared_ptr<ms::ApplicationSession> make_application_session_with_stubs()
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_coordinator, stub_surface_factory, stub_buffer_stream_factory,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           event_sink);
    }
    
    std::shared_ptr<ms::ApplicationSession> make_application_session(
        std::shared_ptr<ms::BufferStreamFactory> const& bstream_factory,
        std::shared_ptr<ms::SurfaceFactory> const& surface_factory)
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_coordinator, surface_factory, bstream_factory,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           event_sink);
    }

    std::shared_ptr<ms::ApplicationSession> make_application_session(
        std::shared_ptr<ms::SurfaceCoordinator> const& surface_coordinator,
        std::shared_ptr<ms::SurfaceFactory> const& surface_factory)
    {
        return std::make_shared<ms::ApplicationSession>(
           surface_coordinator, surface_factory, stub_buffer_stream_factory,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           event_sink);
    }
    std::shared_ptr<ms::ApplicationSession> make_application_session_with_coordinator(
        std::shared_ptr<ms::SurfaceCoordinator> const& surface_coordinator)
    {
        return std::make_shared<ms::ApplicationSession>(
           surface_coordinator, stub_surface_factory, stub_buffer_stream_factory,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           event_sink);
    }
    
    std::shared_ptr<ms::ApplicationSession> make_application_session_with_listener(
        std::shared_ptr<ms::SessionListener> const& session_listener)
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_coordinator, stub_surface_factory, stub_buffer_stream_factory,
           pid, name,
           null_snapshot_strategy,
           session_listener,
           event_sink);
    }


    std::shared_ptr<ms::ApplicationSession> make_application_session_with_buffer_stream_factory(
        std::shared_ptr<ms::BufferStreamFactory> const& buffer_stream_factory)
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_coordinator, stub_surface_factory, buffer_stream_factory,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           event_sink);
    }

    std::shared_ptr<mtd::NullEventSink> const event_sink;
    std::shared_ptr<ms::NullSessionListener> const stub_session_listener;
    std::shared_ptr<StubSurfaceCoordinator> const stub_surface_coordinator;
    std::shared_ptr<ms::SnapshotStrategy> const null_snapshot_strategy;
    std::shared_ptr<mtd::StubBufferStreamFactory> const stub_buffer_stream_factory =
        std::make_shared<mtd::StubBufferStreamFactory>();
    std::shared_ptr<mtd::StubSurfaceFactory> const stub_surface_factory{std::make_shared<mtd::StubSurfaceFactory>()};
    std::shared_ptr<mtd::StubBufferStream> const stub_buffer_stream{std::make_shared<mtd::StubBufferStream>()};
    pid_t pid;
    std::string name;
};

struct MockSurfaceFactory : ms::SurfaceFactory
{
    MOCK_METHOD2(create_surface, std::shared_ptr<ms::Surface>(
        std::shared_ptr<mc::BufferStream> const&, ms::SurfaceCreationParameters const& params));
};
}

TEST_F(ApplicationSession, adds_created_surface_to_coordinator)
{
    using namespace ::testing;

    NiceMock<MockSurfaceFactory> mock_surface_factory;
    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;
    std::shared_ptr<ms::Surface> mock_surface = make_mock_surface();

    EXPECT_CALL(mock_surface_factory, create_surface(_,_))
        .WillOnce(Return(mock_surface));
    EXPECT_CALL(surface_coordinator, add_surface(mock_surface,_,_,_));
    auto session = make_application_session(
        mt::fake_shared(surface_coordinator), mt::fake_shared(mock_surface_factory));

    ms::SurfaceCreationParameters params;
    auto surf = session->create_surface(params);

    session->destroy_surface(surf);
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

    ms::SurfaceCreationParameters params;
    auto surf = session->create_surface(params);

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

        ms::SurfaceCreationParameters params;
        session->create_surface(params);
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

    ms::SurfaceCreationParameters params;
    auto id1 = app_session->create_surface(params);
    auto id2 = app_session->create_surface(params);
    auto id3 = app_session->create_surface(params);

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
    ms::SurfaceCreationParameters params;
    auto id1 = session1->create_surface(params);
    auto surf1 = session1->surface(id1);
    auto id2 = session1->create_surface(params);

    auto session2 = make_application_session_with_stubs();

    EXPECT_THROW({session2->surface_after(surf1);},
                 std::runtime_error);

    session1->destroy_surface(id1);
    session1->destroy_surface(id2);
}

TEST_F(ApplicationSession, surface_after_one_is_self)
{
    auto session = make_application_session_with_stubs();
    ms::SurfaceCreationParameters params;
    auto id = session->create_surface(params);
    auto surf = session->surface(id);

    EXPECT_EQ(surf, session->surface_after(surf));

    session->destroy_surface(id);
}

TEST_F(ApplicationSession, surface_after_cycles_through_all)
{
    auto app_session = make_application_session_with_stubs();

    ms::SurfaceCreationParameters params;

    int const N = 3;
    std::shared_ptr<ms::Surface> surf[N];
    mf::SurfaceId id[N];

    for (int i = 0; i < N; ++i)
    {
        id[i] = app_session->create_surface(params);
        surf[i] = app_session->surface(id[i]);

        if (i > 0)
            ASSERT_NE(surf[i], surf[i-1]);
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
    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;
    auto app_session = make_application_session(mt::fake_shared(surface_coordinator), mt::fake_shared(surface_factory));

    {
        InSequence seq;
        EXPECT_CALL(*mock_surface, hide()).Times(1);
        EXPECT_CALL(*mock_surface, show()).Times(1);
    }

    ms::SurfaceCreationParameters params;
    auto surf = app_session->create_surface(params);

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
    ON_CALL(mock_buffer_stream_factory, create_buffer_stream(_,_,_)).WillByDefault(Return(mock_stream));
    ON_CALL(surface_factory, create_surface(_,_)).WillByDefault(Return(mock_surface));
    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;

    auto const snapshot_strategy = std::make_shared<MockSnapshotStrategy>();

    EXPECT_CALL(*snapshot_strategy, take_snapshot_of(mock_stream, _));

    ms::ApplicationSession app_session(
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(surface_factory),
        mt::fake_shared(mock_buffer_stream_factory),
        pid, name,
        snapshot_strategy,
        std::make_shared<ms::NullSessionListener>(),
        event_sink);

    auto surface = app_session.create_surface(ms::SurfaceCreationParameters{});
    app_session.take_snapshot(ms::SnapshotCallback());
    app_session.destroy_surface(surface);
}

TEST_F(ApplicationSession, returns_null_snapshot_if_no_default_surface)
{
    using namespace ::testing;

    auto snapshot_strategy = std::make_shared<MockSnapshotStrategy>();
    MockSnapshotCallback mock_snapshot_callback;

    ms::ApplicationSession app_session(
        stub_surface_coordinator, stub_surface_factory,
        stub_buffer_stream_factory,
        pid, name,
        snapshot_strategy,
        std::make_shared<ms::NullSessionListener>(),
        event_sink);

    EXPECT_CALL(*snapshot_strategy, take_snapshot_of(_,_)).Times(0);
    EXPECT_CALL(mock_snapshot_callback, operator_call(IsNullSnapshot()));

    app_session.take_snapshot(std::ref(mock_snapshot_callback));
}

TEST_F(ApplicationSession, process_id)
{
    using namespace ::testing;

    pid_t const session_pid{__LINE__};

    ms::ApplicationSession app_session(
        stub_surface_coordinator, stub_surface_factory,
        stub_buffer_stream_factory,
        session_pid, name,
        null_snapshot_strategy,
        std::make_shared<ms::NullSessionListener>(),
        event_sink);

    EXPECT_THAT(app_session.process_id(), Eq(session_pid));
}

TEST_F(ApplicationSession, surface_ids_are_bufferstream_ids)
{
    using namespace ::testing;

    NiceMock<MockSurfaceFactory> mock_surface_factory;
    NiceMock<MockBufferStreamFactory> mock_bufferstream_factory;
    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;
    std::shared_ptr<ms::Surface> mock_surface = make_mock_surface();
    auto stub_bstream = std::make_shared<mtd::StubBufferStream>();
    EXPECT_CALL(mock_bufferstream_factory, create_buffer_stream(_,_,_))
        .WillOnce(Return(stub_bstream));
    EXPECT_CALL(mock_surface_factory, create_surface(std::shared_ptr<mc::BufferStream>(stub_bstream),_))
        .WillOnce(Return(mock_surface));
    auto session = make_application_session(
        mt::fake_shared(mock_bufferstream_factory),
        mt::fake_shared(mock_surface_factory));

    ms::SurfaceCreationParameters params;

    auto id1 = session->create_surface(params);
    EXPECT_THAT(session->get_buffer_stream(mf::BufferStreamId(id1.as_value())), Eq(stub_bstream));
    EXPECT_THAT(session->get_surface(id1), Eq(mock_surface));

    session->destroy_surface(id1);

    EXPECT_THROW({
            session->get_buffer_stream(mf::BufferStreamId(id1.as_value()));
    }, std::runtime_error);
}

TEST_F(ApplicationSession, can_destroy_surface_bstream)
{
    auto session = make_application_session_with_stubs();
    ms::SurfaceCreationParameters params;
    auto id = session->create_surface(params);
    mf::BufferStreamId stream_id(id.as_value());
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
    EXPECT_CALL(mock_bufferstream_factory, create_buffer_stream(_,_,_))
        .WillOnce(Return(streams[0]))
        .WillOnce(Return(streams[1]))
        .WillOnce(Return(streams[2]));

    auto stream_properties = mg::BufferProperties{{8,8}, mir_pixel_format_argb_8888, mg::BufferUsage::hardware};
    auto session = make_application_session(
        mt::fake_shared(mock_bufferstream_factory),
        mt::fake_shared(mock_surface_factory));
    auto stream_id0 = mf::BufferStreamId(session->create_surface(ms::a_surface().of_position({1,1})).as_value());
    auto stream_id1 = session->create_buffer_stream(stream_properties);
    auto stream_id2 = session->create_buffer_stream(stream_properties);

    std::list<ms::StreamInfo> info {
        {streams[2], geom::Displacement{0,3}},
        {streams[0], geom::Displacement{-1,1}},
        {streams[1], geom::Displacement{0,2}}
    };
    EXPECT_CALL(*mock_surface, set_streams(Pointwise(StreamEq(), info)));
    session->configure_streams(*mock_surface, {
        {stream_id2, geom::Displacement{0,3}},
        {stream_id0, geom::Displacement{-1,1}},
        {stream_id1, geom::Displacement{0,2}}
    });
}

TEST_F(ApplicationSession, buffer_stream_constructed_with_requested_parameters)
{
    using namespace ::testing;
    
    geom::Size const buffer_size{geom::Width{1}, geom::Height{1}};

    mtd::StubBufferStream stream;
    MockBufferStreamFactory factory;

    mg::BufferProperties properties(buffer_size, mir_pixel_format_argb_8888, mg::BufferUsage::software);
    
    EXPECT_CALL(factory, create_buffer_stream(_,_,properties)).Times(1)
        .WillOnce(Return(mt::fake_shared(stream)));

    auto session = make_application_session_with_buffer_stream_factory(mt::fake_shared(factory));
    auto id = session->create_buffer_stream(properties);

    EXPECT_TRUE(session->get_buffer_stream(id) != nullptr);
    
    session->destroy_buffer_stream(id);
    
    EXPECT_THROW({
            session->get_buffer_stream(id);
    }, std::runtime_error);
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

    EXPECT_CALL(mock_surface_factory, create_surface(Eq(session->get_buffer_stream(id)),_))
        .WillOnce(Invoke([&](auto bs, auto)
    {
        auto surface = std::make_shared<NiceMock<mtd::MockSurface>>();
        ON_CALL(*surface, primary_buffer_stream())
            .WillByDefault(Return(bs));
        return surface;
    }));

    ms::SurfaceCreationParameters params = ms::SurfaceCreationParameters{}
        .of_name("Aardavks")
        .of_type(mir_surface_type_normal)
        .with_buffer_stream(id);

    auto surface_id = session->create_surface(params);
    auto surface = session->get_surface(surface_id);

    EXPECT_THAT(surface->primary_buffer_stream(), Eq(session->get_buffer_stream(id)));
}

namespace
{
struct ApplicationSessionSender : public ApplicationSession
{
    ApplicationSessionSender() :
        app_session(
        stub_surface_coordinator, stub_surface_factory, stub_buffer_stream_factory,
        pid, name,null_snapshot_strategy, stub_session_listener, mt::fake_shared(sender))
    {
    }

    mtd::MockEventSink sender;
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
