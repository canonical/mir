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

#include "mir/shell/session_manager.h"
#include "mir/surfaces/buffer_bundle.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/shell/focus_sequence.h"
#include "mir/shell/focus_setter.h"
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/session_container.h"
#include "mir/shell/surface_creation_parameters.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface_factory.h"

namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

struct MockFocusSetter: public msh::FocusSetter
{
  MOCK_METHOD1(set_focus_to, void(std::shared_ptr<msh::Session> const&));
};

}

TEST(TestSessionManagerAndFocusSelectionStrategy, cycle_focus)
{
    using namespace ::testing;
    mtd::MockSurfaceFactory surface_factory;
    std::shared_ptr<msh::SessionContainer> container(new msh::SessionContainer());
    msh::RegistrationOrderFocusSequence sequence(container);
    MockFocusSetter focus_changer;
    std::shared_ptr<msh::Session> new_session;

    msh::SessionManager session_manager(
            mt::fake_shared(surface_factory),
            container,
            mt::fake_shared(sequence),
            mt::fake_shared(focus_changer));

    EXPECT_CALL(focus_changer, set_focus_to(_)).Times(3);

    auto session1 = session_manager.open_session("Visual Basic Studio");
    auto session2 = session_manager.open_session("Microsoft Access");
    auto session3 = session_manager.open_session("WordPerfect");

    {
      InSequence seq;
      EXPECT_CALL(focus_changer, set_focus_to(session1)).Times(1);
      EXPECT_CALL(focus_changer, set_focus_to(session2)).Times(1);
      EXPECT_CALL(focus_changer, set_focus_to(session3)).Times(1);
    }

    session_manager.focus_next();
    session_manager.focus_next();
    session_manager.focus_next();
}

TEST(TestSessionManagerAndFocusSelectionStrategy, closing_applications_transfers_focus)
{
    using namespace ::testing;
    mtd::MockSurfaceFactory surface_factory;
    std::shared_ptr<msh::SessionContainer> model(new msh::SessionContainer());
    msh::RegistrationOrderFocusSequence sequence(model);
    MockFocusSetter focus_changer;
    std::shared_ptr<msh::Session> new_session;

    msh::SessionManager session_manager(
        mt::fake_shared(surface_factory),
        model,
        mt::fake_shared(sequence),
        mt::fake_shared(focus_changer));

    EXPECT_CALL(focus_changer, set_focus_to(_)).Times(3);

    auto session1 = session_manager.open_session("Visual Basic Studio");
    auto session2 = session_manager.open_session("Microsoft Access");
    auto session3 = session_manager.open_session("WordPerfect");

    {
      InSequence seq;
      EXPECT_CALL(focus_changer, set_focus_to(session2)).Times(1);
      EXPECT_CALL(focus_changer, set_focus_to(session1)).Times(1);
    }

    session_manager.close_session(session3);
    session_manager.close_session(session2);
}
