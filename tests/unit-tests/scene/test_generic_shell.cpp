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

#include "mir/shell/generic_shell.h"
#include "mir/shell/window_manager.h"

#include "mir/scene/session.h"
#include "mir/scene/null_session_listener.h"
#include "mir/scene/surface_creation_parameters.h"

#include "src/server/scene/default_session_container.h"
#include "src/server/scene/session_event_sink.h"
#include "src/server/scene/session_manager.h"

#include "mir_test_doubles/mock_surface_coordinator.h"
#include "mir_test_doubles/mock_session_listener.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/null_snapshot_strategy.h"
#include "mir_test_doubles/null_prompt_session_manager.h"
#include "mir_test_doubles/stub_input_targeter.h"

#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace mt = mir::test;
namespace mtd = mir::test::doubles;
using namespace ::testing;

namespace
{
MATCHER_P(WeakPtrTo, p, "")
{
  return !arg.owner_before(p) && !p.owner_before(arg);
}

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

struct MockSessionManager : ms::SessionManager
{
    using ms::SessionManager::SessionManager;

    MOCK_METHOD1(set_focus_to, void (std::shared_ptr<ms::Session> const& focus));

    void unmocked_set_focus_to(std::shared_ptr<ms::Session> const& focus)
    { ms::SessionManager::set_focus_to(focus); }
};

struct StubWindowManager : msh::WindowManager
{
//    void add_session(std::shared_ptr<ms::Session> const&) override {}
//
//    void remove_session(std::shared_ptr<ms::Session> const&) override {}
//
//    mf::SurfaceId add_surface(
//        std::shared_ptr<ms::Session> const& session,
//        ms::SurfaceCreationParameters const& params,
//        std::function<mf::SurfaceId(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params)> const& build) override
//    {
//        return build(session, params);
//    }
//
    void remove_surface(
        std::shared_ptr<ms::Session> const&,
        std::weak_ptr<ms::Surface> const&) override {}

    void add_display(geom::Rectangle const&) override {}

    void remove_display(geom::Rectangle const&) override {}

    bool handle_key_event(MirKeyInputEvent const*) override { return false; }

    bool handle_touch_event(MirTouchInputEvent const*) override { return false; }

    bool handle_pointer_event(MirPointerInputEvent const*) override { return false; }

    int handle_set_state(std::shared_ptr<ms::Surface> const&, MirSurfaceState value) override { return value; }
};

struct MockWindowManager : StubWindowManager
{
    MockWindowManager()
    {
        ON_CALL(*this, add_surface(_,_,_)).WillByDefault(Invoke(
            [](std::shared_ptr<ms::Session> const& session,
                ms::SurfaceCreationParameters const& params,
                std::function<mf::SurfaceId(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params)> const& build)
                { return build(session, params); }));
    }

    MOCK_METHOD1(add_session, void (std::shared_ptr<ms::Session> const&));
    MOCK_METHOD1(remove_session, void (std::shared_ptr<ms::Session> const&));

    MOCK_METHOD3(add_surface, mf::SurfaceId(
        std::shared_ptr<ms::Session> const& session,
        ms::SurfaceCreationParameters const& params,
        std::function<mf::SurfaceId(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& params)> const& build));

    MOCK_METHOD2(remove_surface, void(std::shared_ptr<ms::Session> const&, std::weak_ptr<ms::Surface> const&));
};

using NiceMockWindowManager = NiceMock<MockWindowManager>;

struct GenericShell : Test
{
    NiceMock<mtd::MockSurface> mock_surface;
    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;
    NiceMock<MockSessionContainer> session_container;
    NiceMock<MockSessionEventSink> session_event_sink;
    NiceMock<mtd::MockSessionListener> session_listener;

    NiceMock<MockSessionManager> session_manager{
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(session_container),
        std::make_shared<mtd::NullSnapshotStrategy>(),
        mt::fake_shared(session_event_sink),
        mt::fake_shared(session_listener)};

    mtd::StubInputTargeter input_targeter;
    std::shared_ptr<MockWindowManager> wm;

    msh::GenericShell shell{
        mt::fake_shared(input_targeter),
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(session_manager),
        std::make_shared<mtd::NullPromptSessionManager>(),
        [this](msh::FocusController*) { return wm = std::make_shared<NiceMockWindowManager>(); }};

    void SetUp() override
    {
        ON_CALL(session_container, successor_of(_)).WillByDefault(Return((std::shared_ptr<ms::Session>())));
        ON_CALL(session_manager, set_focus_to(_)).
            WillByDefault(Invoke(&session_manager, &MockSessionManager::unmocked_set_focus_to));
        ON_CALL(surface_coordinator, add_surface(_,_))
            .WillByDefault(Return(mt::fake_shared(mock_surface)));
    }
};
}

TEST_F(GenericShell, open_session_adds_session_to_window_manager)
{
    std::shared_ptr<ms::Session> new_session;

    InSequence s;
    EXPECT_CALL(session_container, insert_session(_)).Times(1);
    EXPECT_CALL(*wm, add_session(_)).WillOnce(SaveArg<0>(&new_session));

    auto session = shell.open_session(__LINE__, "Visual Basic Studio", std::shared_ptr<mf::EventSink>());
    EXPECT_EQ(session, new_session);
}

TEST_F(GenericShell, close_session_removes_session_from_window_manager)
{
    std::shared_ptr<ms::Session> old_session;

    InSequence s;
    EXPECT_CALL(session_container, insert_session(_));
    EXPECT_CALL(*wm, remove_session(_)).WillOnce(SaveArg<0>(&old_session));

    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    shell.close_session(session);
    EXPECT_EQ(session, old_session);
}

TEST_F(GenericShell, close_session_notifies_session_event_sink)
{
    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());

    InSequence s;
    EXPECT_CALL(session_event_sink, handle_session_stopping(session1));
    EXPECT_CALL(session_event_sink, handle_session_stopping(session));

    shell.close_session(session1);
    shell.close_session(session);
}

TEST_F(GenericShell, focus_controller_set_focus_to_notifies_session_event_sink)
{
    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());

    InSequence s;
    EXPECT_CALL(session_event_sink, handle_focus_change(session1));
    EXPECT_CALL(session_event_sink, handle_focus_change(session));
    EXPECT_CALL(session_event_sink, handle_no_focus());

    msh::FocusController& focus_controller = shell;
    focus_controller.set_focus_to(session1, {});
    focus_controller.set_focus_to(session, {});
    focus_controller.set_focus_to({}, {});
}

TEST_F(GenericShell, create_surface_provides_create_parameters_to_window_manager)
{
    std::shared_ptr<ms::Session> session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = ms::a_surface();

    EXPECT_CALL(*wm, add_surface(session, Ref(params), _));

    shell.create_surface(session, params);
}

TEST_F(GenericShell, create_surface_allows_window_manager_to_set_create_parameters)
{
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto params = ms::a_surface();
    auto placed_params = params;
    placed_params.size.width = geom::Width{100};

    EXPECT_CALL(surface_coordinator, add_surface(placed_params, _));

    EXPECT_CALL(*wm, add_surface(session, Ref(params), _)).WillOnce(Invoke(
        [&](std::shared_ptr<ms::Session> const& session,
            ms::SurfaceCreationParameters const&,
            std::function<mf::SurfaceId(std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const&)> const& build)
            { return build(session, placed_params); }));

    shell.create_surface(session, params);
}

TEST_F(GenericShell, destroy_surface_removes_surface_from_window_manager)
{
    auto const params = ms::a_surface();
    std::shared_ptr<ms::Session> const session =
        shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());

    auto const surface_id = shell.create_surface(session, params);
    auto const weak_surface = session->surface(surface_id);

    EXPECT_CALL(*wm, remove_surface(session, WeakPtrTo(weak_surface)));

    shell.destroy_surface(session, surface_id);
}
