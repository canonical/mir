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
#include "mir/shell/focus_setter.h"
#include "src/server/scene/default_session_container.h"
#include "mir/scene/null_session_listener.h"
#include "mir/compositor/buffer_stream.h"
#include "src/server/scene/basic_surface.h"
#include "mir/scene/surface_creation_parameters.h"

#include "mir_test/gmock_fixes.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface_coordinator.h"
#include "mir_test_doubles/mock_focus_setter.h"
#include "mir_test_doubles/null_snapshot_strategy.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/null_session_event_sink.h"
#include "mir_test_doubles/null_prompt_session_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

struct TestDefaultShellAndFocusSelectionStrategy : public testing::Test
{
    mtd::MockSurfaceCoordinator surface_coordinator; // TODO this isn't used as a mock
    ms::DefaultSessionContainer container;
    mtd::MockFocusSetter focus_setter;
    std::shared_ptr<mf::Session> new_session;
    ms::NullSessionListener session_listener;
    ms::SessionManager session_manager
        {
            mt::fake_shared(surface_coordinator),
            mt::fake_shared(container),
            std::make_shared<mtd::NullSnapshotStrategy>(),
            std::make_shared<mtd::NullSessionEventSink>(),
            mt::fake_shared(session_listener),
            std::make_shared<mtd::NullPromptSessionManager>()
        };

    msh::DefaultShell shell{mt::fake_shared(focus_setter), mt::fake_shared(session_manager)};
};
}

TEST_F(TestDefaultShellAndFocusSelectionStrategy, cycle_focus)
{
    using namespace ::testing;

    EXPECT_CALL(focus_setter, set_focus_to(_)).Times(3);

    auto session1 = shell.open_session(__LINE__, "Visual Basic Studio", std::make_shared<mtd::NullEventSink>());
    auto session2 = shell.open_session(__LINE__, "Microsoft Access", std::make_shared<mtd::NullEventSink>());
    auto session3 = shell.open_session(__LINE__, "WordPerfect", std::make_shared<mtd::NullEventSink>());

    Mock::VerifyAndClearExpectations(&focus_setter);

    {
      InSequence seq;
      EXPECT_CALL(focus_setter, set_focus_to(Eq(session1))).Times(1);
      EXPECT_CALL(focus_setter, set_focus_to(Eq(session2))).Times(1);
      EXPECT_CALL(focus_setter, set_focus_to(Eq(session3))).Times(1);
    }

    shell.focus_next();
    shell.focus_next();
    shell.focus_next();

    Mock::VerifyAndClearExpectations(&focus_setter);

    // Possible change of focus while sessions are closed on shutdown
    EXPECT_CALL(focus_setter, set_focus_to(_)).Times(AtLeast(0));
}

TEST_F(TestDefaultShellAndFocusSelectionStrategy, closing_applications_transfers_focus)
{
    using namespace ::testing;

    EXPECT_CALL(focus_setter, set_focus_to(_)).Times(3);

    auto session1 = shell.open_session(__LINE__, "Visual Basic Studio", std::make_shared<mtd::NullEventSink>());
    auto session2 = shell.open_session(__LINE__, "Microsoft Access", std::make_shared<mtd::NullEventSink>());
    auto session3 = shell.open_session(__LINE__, "WordPerfect", std::make_shared<mtd::NullEventSink>());

    Mock::VerifyAndClearExpectations(&focus_setter);

    {
      InSequence seq;
      EXPECT_CALL(focus_setter, set_focus_to(Eq(session2))).Times(1);
      EXPECT_CALL(focus_setter, set_focus_to(Eq(session1))).Times(1);
    }

    shell.close_session(session3);
    shell.close_session(session2);

    Mock::VerifyAndClearExpectations(&focus_setter);

    // Possible change of focus while sessions are closed on shutdown
    EXPECT_CALL(focus_setter, set_focus_to(_)).Times(AtLeast(0));
}
