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

#include "mir/frontend/application_manager.h"
#include "mir/compositor/buffer_bundle.h"
#include "mir/surfaces/surface_controller.h"
#include "mir/surfaces/surface_stack.h"
#include "mir/surfaces/surface.h"
#include "mir/frontend/services/surface_factory.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/frontend/registration_order_focus_strategy.h"
#include "mir/frontend/application_session_model.h"


#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"
#include "mir_test/empty_deleter.h"

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mfs = mir::frontend::services;
namespace ms = mir::surfaces;

namespace
{
struct MockApplicationSurfaceOrganiser : public ms::ApplicationSurfaceOrganiser
{
    MOCK_METHOD1(create_surface, std::weak_ptr<ms::Surface>(const ms::SurfaceCreationParameters&));
    MOCK_METHOD1(destroy_surface, void(std::weak_ptr<ms::Surface> surface));
    MOCK_METHOD2(hide_surface, void(std::weak_ptr<ms::Surface>,bool));
};

struct MockFocusMechanism: public mf::ApplicationFocusMechanism
{
  MOCK_METHOD2(focus, void(std::shared_ptr<mf::ApplicationSessionContainer>, std::shared_ptr<mf::ApplicationSession>));
};

}

TEST(TestApplicationManagerAndFocusStrategy, closing_applications_transfers_focus)
{
    using namespace ::testing;
    MockApplicationSurfaceOrganiser organiser;
    std::shared_ptr<mf::ApplicationSessionModel> model(new mf::ApplicationSessionModel());
    mf::RegistrationOrderFocusStrategy strategy(model);
    MockFocusMechanism mechanism;
    std::shared_ptr<mf::ApplicationSession> new_session;

    mf::ApplicationManager app_manager(std::shared_ptr<ms::ApplicationSurfaceOrganiser>(&organiser, mir::EmptyDeleter()), 
                                       model,
                                       std::shared_ptr<mf::ApplicationFocusStrategy>(&strategy, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::ApplicationFocusMechanism>(&mechanism, mir::EmptyDeleter()));
    
    EXPECT_CALL(mechanism, focus(_,_)).Times(3);

    auto session1 = app_manager.open_session("Visual Basic Studio");
    auto session2 = app_manager.open_session("Microsoft Access");
    auto session3 = app_manager.open_session("WordPerfect");
    
    {
      InSequence seq;
      EXPECT_CALL(mechanism, focus(_,session2)).Times(1);
      EXPECT_CALL(mechanism, focus(_,session1)).Times(1);
    }
    
    app_manager.close_session(session3);
    app_manager.close_session(session2);
}
