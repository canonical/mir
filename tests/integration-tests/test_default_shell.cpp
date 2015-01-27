/*
 * Copyright © 2012-2015 Canonical Ltd.
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

#include "src/server/shell/default_shell.h"
#include "src/server/scene/session_manager.h"
#include "mir/scene/session.h"
#include "src/server/scene/default_session_container.h"
#include "mir/scene/null_session_listener.h"

#include "mir_test/gmock_fixes.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_scene_session.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/mock_surface_coordinator.h"
#include "mir_test_doubles/null_snapshot_strategy.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/null_session_event_sink.h"
#include "mir_test_doubles/null_prompt_session_manager.h"
#include "mir_test_doubles/mock_input_targeter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace
{
struct MockSessionManager : ms::SessionManager
{
    using ms::SessionManager::SessionManager;

    MOCK_METHOD1(set_focus_to, void (std::shared_ptr<ms::Session> const& focus));
};

struct TestDefaultShellAndFocusSelectionStrategy : public testing::Test
{
    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;
    ms::DefaultSessionContainer container;
    NiceMock<mtd::MockInputTargeter> input_targeter;
    std::shared_ptr<mf::Session> new_session;
    ms::NullSessionListener session_listener;
    NiceMock<MockSessionManager> session_manager
        {
            mt::fake_shared(surface_coordinator),
            mt::fake_shared(container),
            std::make_shared<mtd::NullSnapshotStrategy>(),
            std::make_shared<mtd::NullSessionEventSink>(),
            mt::fake_shared(session_listener),
            std::make_shared<mtd::NullPromptSessionManager>()
        };

    msh::DefaultShell shell{
        mt::fake_shared(input_targeter),
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(session_manager)};
};
}

TEST_F(TestDefaultShellAndFocusSelectionStrategy, cycle_focus)
{
    EXPECT_CALL(session_manager, set_focus_to(_)).Times(3);

    auto session1 = shell.open_session(__LINE__, "Visual Basic Studio", std::make_shared<mtd::NullEventSink>());
    auto session2 = shell.open_session(__LINE__, "Microsoft Access", std::make_shared<mtd::NullEventSink>());
    auto session3 = shell.open_session(__LINE__, "WordPerfect", std::make_shared<mtd::NullEventSink>());

    Mock::VerifyAndClearExpectations(&session_manager);

    {
      InSequence seq;
      EXPECT_CALL(session_manager, set_focus_to(Eq(session1))).Times(1);
      EXPECT_CALL(session_manager, set_focus_to(Eq(session2))).Times(1);
      EXPECT_CALL(session_manager, set_focus_to(Eq(session3))).Times(1);
    }

    shell.focus_next();
    shell.focus_next();
    shell.focus_next();

    Mock::VerifyAndClearExpectations(&session_manager);

    // Possible change of focus while sessions are closed on shutdown
    EXPECT_CALL(session_manager, set_focus_to(_)).Times(AnyNumber());
}

TEST_F(TestDefaultShellAndFocusSelectionStrategy, closing_applications_transfers_focus)
{
    using namespace ::testing;

    EXPECT_CALL(session_manager, set_focus_to(_)).Times(3);

    auto session1 = shell.open_session(__LINE__, "Visual Basic Studio", std::make_shared<mtd::NullEventSink>());
    auto session2 = shell.open_session(__LINE__, "Microsoft Access", std::make_shared<mtd::NullEventSink>());
    auto session3 = shell.open_session(__LINE__, "WordPerfect", std::make_shared<mtd::NullEventSink>());

    Mock::VerifyAndClearExpectations(&session_manager);

    {
      InSequence seq;
      EXPECT_CALL(session_manager, set_focus_to(Eq(session2))).Times(1);
      EXPECT_CALL(session_manager, set_focus_to(Eq(session1))).Times(1);
    }

    shell.close_session(session3);
    shell.close_session(session2);

    Mock::VerifyAndClearExpectations(&session_manager);

    // Possible change of focus while sessions are closed on shutdown
    EXPECT_CALL(session_manager, set_focus_to(_)).Times(AtLeast(0));
}

TEST_F(TestDefaultShellAndFocusSelectionStrategy, mechanism_notifies_default_surface_of_focus_changes)
{
    using namespace ::testing;

    NiceMock<mtd::MockSceneSession> app1, app2;
    auto const mock_surface1 = std::make_shared<NiceMock<mtd::MockSurface>>();
    auto const mock_surface2 = std::make_shared<NiceMock<mtd::MockSurface>>();

    ON_CALL(app1, default_surface()).WillByDefault(Return(mock_surface1));
    ON_CALL(app2, default_surface()).WillByDefault(Return(mock_surface2));

    InSequence seq;
    EXPECT_CALL(surface_coordinator, raise(_)).Times(1);
    EXPECT_CALL(*mock_surface1, configure(mir_surface_attrib_focus, mir_surface_focused)).Times(1);
    EXPECT_CALL(surface_coordinator, raise(_)).Times(1);
    EXPECT_CALL(*mock_surface1, configure(mir_surface_attrib_focus, mir_surface_unfocused)).Times(1);
    EXPECT_CALL(*mock_surface2, configure(mir_surface_attrib_focus, mir_surface_focused)).Times(1);

    shell.set_focus_to(mt::fake_shared(app1));
    shell.set_focus_to(mt::fake_shared(app2));
}

TEST_F(TestDefaultShellAndFocusSelectionStrategy, sets_input_focus)
{
    NiceMock<mtd::MockSceneSession> app1;
    NiceMock<mtd::MockSurface> mock_surface;

    {
        InSequence seq;
        EXPECT_CALL(app1, default_surface()).Times(1)
            .WillOnce(Return(mt::fake_shared(mock_surface)));
        EXPECT_CALL(app1, default_surface()).Times(1)
            .WillOnce(Return(std::shared_ptr<ms::Surface>()));
    }

    {
        InSequence seq;
        EXPECT_CALL(mock_surface, take_input_focus(_)).Times(1);
        // When we have no default surface.
        EXPECT_CALL(input_targeter, focus_cleared()).Times(1);
        // When we have no session.
        EXPECT_CALL(input_targeter, focus_cleared()).Times(1);
    }
    EXPECT_CALL(surface_coordinator, raise(_)).Times(1);

    shell.set_focus_to(mt::fake_shared(app1));
    shell.set_focus_to(mt::fake_shared(app1));
    shell.set_focus_to(std::shared_ptr<ms::Session>());
}

TEST_F(TestDefaultShellAndFocusSelectionStrategy, notifies_surface_of_focus_change_after_it_has_taken_the_focus)
{
    NiceMock<mtd::MockSceneSession> app;
    auto const mock_surface = std::make_shared<NiceMock<mtd::MockSurface>>();

    ON_CALL(app, default_surface()).WillByDefault(Return(mock_surface));


    InSequence seq;
    EXPECT_CALL(*mock_surface, take_input_focus(_));
    EXPECT_CALL(*mock_surface, configure(mir_surface_attrib_focus, mir_surface_focused)).Times(1);

    shell.set_focus_to(mt::fake_shared(app));
}
