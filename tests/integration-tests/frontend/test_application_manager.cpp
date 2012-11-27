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

#include "mir/frontend/session_manager.h"
#include "mir/compositor/buffer_bundle.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/frontend/focus_sequence.h"
#include "mir/frontend/focus.h"
#include "mir/frontend/registration_order_focus_sequence.h"
#include "mir/frontend/application_session_model.h"


#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"
#include "mir_test/empty_deleter.h"
#include "mir_test/mock_application_surface_organiser.h"

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;

namespace
{

struct MockFocusMechanism: public mf::Focus
{
  MOCK_METHOD1(set_focus_to, void(std::shared_ptr<mf::Session> const&));
};

}

TEST(TestApplicationManagerAndFocusSelectionStrategy, cycle_focus)
{
    using namespace ::testing;
    mf::MockSurfaceOrganiser organiser;
    std::shared_ptr<mf::TheSessionContainerImplementation> model(new mf::TheSessionContainerImplementation());
    mf::RegistrationOrderFocusSequence strategy(model);
    MockFocusMechanism mechanism;
    std::shared_ptr<mf::Session> new_session;

    mf::SessionManager session_manager(std::shared_ptr<mf::SurfaceOrganiser>(&organiser, mir::EmptyDeleter()), 
                                       model,
                                       std::shared_ptr<mf::FocusSequence>(&strategy, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::Focus>(&mechanism, mir::EmptyDeleter()));
    
    EXPECT_CALL(mechanism, set_focus_to(_)).Times(3);

    auto session1 = session_manager.open_session("Visual Basic Studio");
    auto session2 = session_manager.open_session("Microsoft Access");
    auto session3 = session_manager.open_session("WordPerfect");
    
    {
      InSequence seq;
      EXPECT_CALL(mechanism, set_focus_to(session1)).Times(1);
      EXPECT_CALL(mechanism, set_focus_to(session2)).Times(1);
      EXPECT_CALL(mechanism, set_focus_to(session3)).Times(1);
    }
    
    session_manager.focus_next();
    session_manager.focus_next();
    session_manager.focus_next();
}

TEST(TestApplicationManagerAndFocusSelectionStrategy, closing_applications_transfers_focus)
{
    using namespace ::testing;
    mf::MockSurfaceOrganiser organiser;
    std::shared_ptr<mf::TheSessionContainerImplementation> model(new mf::TheSessionContainerImplementation());
    mf::RegistrationOrderFocusSequence strategy(model);
    MockFocusMechanism mechanism;
    std::shared_ptr<mf::Session> new_session;

    mf::SessionManager session_manager(std::shared_ptr<mf::SurfaceOrganiser>(&organiser, mir::EmptyDeleter()),
                                       model,
                                       std::shared_ptr<mf::FocusSequence>(&strategy, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::Focus>(&mechanism, mir::EmptyDeleter()));
    
    EXPECT_CALL(mechanism, set_focus_to(_)).Times(3);

    auto session1 = session_manager.open_session("Visual Basic Studio");
    auto session2 = session_manager.open_session("Microsoft Access");
    auto session3 = session_manager.open_session("WordPerfect");
    
    {
      InSequence seq;
      EXPECT_CALL(mechanism, set_focus_to(session2)).Times(1);
      EXPECT_CALL(mechanism, set_focus_to(session1)).Times(1);
    }
    
    session_manager.close_session(session3);
    session_manager.close_session(session2);
}
