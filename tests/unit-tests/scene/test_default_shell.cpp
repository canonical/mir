/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: ALan Griffiths <alan.griffiths@canonical.com>
 */

#include "src/server/shell/default_shell.h"

#include "mir/scene/session.h"
#include "mir/scene/null_session_listener.h"
#include "mir/scene/placement_strategy.h"
#include "mir/scene/prompt_session.h"
#include "mir/scene/prompt_session_creation_parameters.h"
#include "mir/scene/surface_creation_parameters.h"

#include "src/server/scene/default_session_container.h"
#include "src/server/scene/session_event_sink.h"
#include "src/server/scene/session_manager.h"

#include "mir_test_doubles/mock_surface_coordinator.h"
#include "mir_test_doubles/mock_session_listener.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/null_snapshot_strategy.h"
#include "mir_test_doubles/null_surface_configurator.h"
#include "mir_test_doubles/null_prompt_session_manager.h"
#include "mir_test_doubles/stub_input_targeter.h"

#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace msh = mir::shell;
using namespace ::testing;

namespace
{
struct MockSessionContainer : public ms::SessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<ms::Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<ms::Session> const&));
    MOCK_CONST_METHOD1(successor_of, std::shared_ptr<ms::Session>(std::shared_ptr<ms::Session> const&));
    MOCK_CONST_METHOD1(for_each, void(std::function<void(std::shared_ptr<ms::Session> const&)>));
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
    ~MockSessionContainer() noexcept {}
};

struct MockSessionEventSink : public ms::SessionEventSink
{
    MOCK_METHOD1(handle_focus_change, void(std::shared_ptr<ms::Session> const& session));
    MOCK_METHOD0(handle_no_focus, void());
    MOCK_METHOD1(handle_session_stopping, void(std::shared_ptr<ms::Session> const& session));
};
}


namespace
{
struct MockPlacementStrategy : public ms::PlacementStrategy
{
    MockPlacementStrategy()
    {
        ON_CALL(*this, place(_, _)).WillByDefault(ReturnArg<1>());
    }

    MOCK_METHOD2(place, ms::SurfaceCreationParameters(ms::Session const&, ms::SurfaceCreationParameters const&));
};

struct MockSessionManager : ms::SessionManager
{
    using ms::SessionManager::SessionManager;

    MOCK_METHOD1(set_focus_to, void (std::shared_ptr<ms::Session> const& focus));

    void unmocked_set_focus_to(std::shared_ptr<ms::Session> const& focus)
    { ms::SessionManager::set_focus_to(focus); }
};

struct DefaultShell : Test
{
    NiceMock<mtd::MockSurface> mock_surface;
    MockPlacementStrategy placement_strategy;
    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;
    NiceMock<MockSessionContainer> container;
    NiceMock<MockSessionEventSink> session_event_sink;
    NiceMock<mtd::MockSessionListener> session_listener;

    NiceMock<MockSessionManager> session_manager{
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(container),
        std::make_shared<mtd::NullSnapshotStrategy>(),
        mt::fake_shared(session_event_sink),
        mt::fake_shared(session_listener)};

    mtd::StubInputTargeter input_targeter;
    msh::DefaultShell shell{
        mt::fake_shared(input_targeter),
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(session_manager),
        std::make_shared<mtd::NullPromptSessionManager>(),
        mt::fake_shared(placement_strategy),
        std::make_shared<mtd::NullSurfaceConfigurator>()};

    void SetUp() override
    {
        ON_CALL(container, successor_of(_)).WillByDefault(Return((std::shared_ptr<ms::Session>())));
        ON_CALL(session_manager, set_focus_to(_)).
            WillByDefault(Invoke(&session_manager, &MockSessionManager::unmocked_set_focus_to));
        ON_CALL(surface_coordinator, add_surface(_,_))
            .WillByDefault(Return(mt::fake_shared(mock_surface)));
    }
};
}

TEST_F(DefaultShell, new_applications_receive_focus)
{
    std::shared_ptr<ms::Session> new_session;

    EXPECT_CALL(container, insert_session(_)).Times(1);
    EXPECT_CALL(session_manager, set_focus_to(_)).WillOnce(SaveArg<0>(&new_session));

    auto session = shell.open_session(__LINE__, "Visual Basic Studio", std::shared_ptr<mf::EventSink>());
    EXPECT_EQ(session, new_session);
}

TEST_F(DefaultShell, session_listener_is_notified_of_focus)
{
    EXPECT_CALL(session_listener, focused(_)).Times(1);
    EXPECT_CALL(session_listener, unfocused()).Times(1);

    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    shell.close_session(session);
}

TEST_F(DefaultShell, session_event_sink_is_notified_of_focus)
{
    EXPECT_CALL(session_event_sink, handle_focus_change(_)).Times(2);

    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());

    Mock::VerifyAndClearExpectations(&session_event_sink);

    InSequence s;
    EXPECT_CALL(session_event_sink, handle_session_stopping(_)).Times(1);
    EXPECT_CALL(container, successor_of(_)).
        WillOnce(Return(std::dynamic_pointer_cast<ms::Session>(session)));
    EXPECT_CALL(session_event_sink, handle_focus_change(_)).Times(1);
    EXPECT_CALL(session_event_sink, handle_session_stopping(_)).Times(1);
    EXPECT_CALL(container, successor_of(_)).
        WillOnce(Return(std::shared_ptr<ms::Session>()));
    EXPECT_CALL(session_event_sink, handle_no_focus()).Times(1);

    shell.close_session(session1);
    shell.close_session(session);
}

TEST_F(DefaultShell, offers_create_surface_parameters_to_placement_strategy)
{
    std::shared_ptr<ms::Session> session;
    EXPECT_CALL(session_manager, set_focus_to(_)).WillOnce(SaveArg<0>(&session));
    shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = ms::a_surface();
    EXPECT_CALL(placement_strategy, place(Ref(*session), Ref(params)));

    shell.create_surface(session, params);
}

TEST_F(DefaultShell, forwards_create_surface_parameters_from_placement_strategy_to_model)
{
    std::shared_ptr<ms::Session> session;
    EXPECT_CALL(session_manager, set_focus_to(_)).WillOnce(SaveArg<0>(&session));
    shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = ms::a_surface();
    auto placed_params = params;
    placed_params.size.width = geom::Width{100};

    EXPECT_CALL(placement_strategy, place(_, Ref(params))).Times(1)
        .WillOnce(Return(placed_params));
    EXPECT_CALL(surface_coordinator, add_surface(placed_params, _));

    shell.create_surface(session, params);
}
