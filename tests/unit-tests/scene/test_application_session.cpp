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
#include "mir/graphics/buffer.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/scene/null_session_listener.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface_coordinator.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/mock_session_listener.h"
#include "mir_test_doubles/stub_display_configuration.h"
#include "mir_test_doubles/null_snapshot_strategy.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/null_prompt_session.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mi = mir::input;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
static std::shared_ptr<mtd::MockSurface> make_mock_surface()
{
    return std::make_shared<testing::NiceMock<mtd::MockSurface> >();
}

class MockSnapshotStrategy : public ms::SnapshotStrategy
{
public:
    ~MockSnapshotStrategy() noexcept {}

    MOCK_METHOD2(take_snapshot_of,
                void(std::shared_ptr<ms::SurfaceBufferAccess> const&,
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

struct StubSurfaceCoordinator : public ms::SurfaceCoordinator
{
    void raise(std::weak_ptr<ms::Surface> const&) override
    {
    }
    std::shared_ptr<ms::Surface> add_surface(ms::SurfaceCreationParameters const&,
        ms::Session*) override
    {
        return make_mock_surface();
    }
    void remove_surface(std::weak_ptr<ms::Surface> const&) override
    {
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
           stub_surface_coordinator,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           event_sink);
    }
    
    std::shared_ptr<ms::ApplicationSession> make_application_session_with_coordinator(
        std::shared_ptr<ms::SurfaceCoordinator> const& surface_coordinator)
    {
        return std::make_shared<ms::ApplicationSession>(
           surface_coordinator,
           pid, name,
           null_snapshot_strategy,
           stub_session_listener,
           event_sink);
    }
    
    std::shared_ptr<ms::ApplicationSession> make_application_session_with_listener(
        std::shared_ptr<ms::SessionListener> const& session_listener)
    {
        return std::make_shared<ms::ApplicationSession>(
           stub_surface_coordinator,
           pid, name,
           null_snapshot_strategy,
           session_listener,
           event_sink);
    }

    std::shared_ptr<mtd::NullEventSink> const event_sink;
    std::shared_ptr<ms::NullSessionListener> const stub_session_listener;
    std::shared_ptr<StubSurfaceCoordinator> const stub_surface_coordinator;
    std::shared_ptr<ms::SnapshotStrategy> const null_snapshot_strategy;
    
    pid_t pid;
    std::string name;
};

}

TEST_F(ApplicationSession, uses_coordinator_to_create_surface)
{
    using namespace ::testing;

    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;
    auto mock_surface = make_mock_surface();
    EXPECT_CALL(surface_coordinator, add_surface(_, _))
        .WillOnce(Return(mock_surface));

    auto session = make_application_session_with_coordinator(mt::fake_shared(surface_coordinator));

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

TEST_F(ApplicationSession, session_visbility_propagates_to_surfaces)
{
    using namespace ::testing;

    auto mock_surface = make_mock_surface();

    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;
    ON_CALL(surface_coordinator, add_surface(_, _)).WillByDefault(Return(mock_surface));

    auto app_session = make_application_session_with_coordinator(mt::fake_shared(surface_coordinator));

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
    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;

    EXPECT_CALL(surface_coordinator, add_surface(_, _))
        .WillOnce(Return(mock_surface));

    auto const default_surface_buffer_access =
        std::static_pointer_cast<ms::SurfaceBufferAccess>(mock_surface);
    auto const snapshot_strategy = std::make_shared<MockSnapshotStrategy>();

    EXPECT_CALL(*snapshot_strategy,
                take_snapshot_of(default_surface_buffer_access, _));

    ms::ApplicationSession app_session(
        mt::fake_shared(surface_coordinator),                                       
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
        stub_surface_coordinator,
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
        stub_surface_coordinator,
        session_pid, name,
        null_snapshot_strategy,
        std::make_shared<ms::NullSessionListener>(),
        event_sink);

    EXPECT_THAT(app_session.process_id(), Eq(session_pid));
}

namespace
{
class MockEventSink : public mf::EventSink
{
public:
    MOCK_METHOD1(handle_event, void(MirEvent const&));
    MOCK_METHOD1(handle_lifecycle_event, void(MirLifecycleState));
    MOCK_METHOD1(handle_display_config_change, void(mir::graphics::DisplayConfiguration const&));
};
struct ApplicationSessionSender : public ApplicationSession
{
    ApplicationSessionSender()
        : app_session(stub_surface_coordinator,pid, name,null_snapshot_strategy, stub_session_listener, mt::fake_shared(sender))
    {
    }

    MockEventSink sender;
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
