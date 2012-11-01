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

#include "mir/compositor/buffer_bundle.h"
#include "mir/frontend/application_manager.h"
#include "mir/frontend/application_session_container.h"
#include "mir/frontend/application_session.h"
#include "mir/frontend/application_focus_strategy.h"
#include "mir/frontend/application_focus_mechanism.h"
#include "mir/surfaces/surface.h"
#include "mir_test/mock_buffer_bundle.h"
#include "mir_test/empty_deleter.h"
#include "mir_test/mock_application_surface_organiser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;

namespace
{

struct MockApplicationSessionModel : public mf::ApplicationSessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<mf::ApplicationSession>));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<mf::ApplicationSession>));
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
    MOCK_METHOD0(iterator, std::shared_ptr<mf::ApplicationSessionContainer::LockingIterator>());
};

struct MockFocusStrategy: public mf::ApplicationFocusStrategy
{
    MOCK_METHOD1(next_focus_app, std::weak_ptr<mf::ApplicationSession>(std::shared_ptr<mf::ApplicationSession>));
    MOCK_METHOD1(previous_focus_app, std::weak_ptr<mf::ApplicationSession>(std::shared_ptr<mf::ApplicationSession>));
};
  
struct MockFocusMechanism: public mf::ApplicationFocusMechanism
{
    MOCK_METHOD2(focus, void(std::shared_ptr<mf::ApplicationSessionContainer>, std::shared_ptr<mf::ApplicationSession>));
};

}

TEST(ApplicationManager, open_and_close_session)
{
    using namespace ::testing;
    ms::MockApplicationSurfaceOrganiser organiser;
    MockApplicationSessionModel model;
    MockFocusStrategy strategy;
    MockFocusMechanism mechanism;

    mf::ApplicationManager app_manager(std::shared_ptr<ms::ApplicationSurfaceOrganiser>(&organiser, mir::EmptyDeleter()), 
                                       std::shared_ptr<mf::ApplicationSessionContainer>(&model, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::ApplicationFocusStrategy>(&strategy, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::ApplicationFocusMechanism>(&mechanism, mir::EmptyDeleter()));
    
    EXPECT_CALL(model, insert_session(_)).Times(1);
    EXPECT_CALL(model, remove_session(_)).Times(1);
    EXPECT_CALL(mechanism, focus(_,_));
    EXPECT_CALL(mechanism, focus(_,std::shared_ptr<mf::ApplicationSession>())).Times(1);

    EXPECT_CALL(strategy, previous_focus_app(_)).WillOnce(Return((std::shared_ptr<mf::ApplicationSession>())));

    auto session = app_manager.open_session("Visual Basic Studio");
    app_manager.close_session(session);
}

TEST(ApplicationManager, closing_session_removes_surfaces)
{
    using namespace ::testing;
    ms::MockApplicationSurfaceOrganiser organiser;
    MockApplicationSessionModel model;
    MockFocusStrategy strategy;
    MockFocusMechanism mechanism;

    mf::ApplicationManager app_manager(std::shared_ptr<ms::ApplicationSurfaceOrganiser>(&organiser, mir::EmptyDeleter()), 
                                       std::shared_ptr<mf::ApplicationSessionContainer>(&model, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::ApplicationFocusStrategy>(&strategy, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::ApplicationFocusMechanism>(&mechanism, mir::EmptyDeleter()));
    
    EXPECT_CALL(organiser, create_surface(_)).Times(1);
    std::shared_ptr<mc::BufferBundle> buffer_bundle(
        new mc::MockBufferBundle());
    std::shared_ptr<ms::Surface> dummy_surface(
        new ms::Surface(
            ms::a_surface().name,
            buffer_bundle));
    ON_CALL(organiser, create_surface(_)).WillByDefault(Return(dummy_surface));

    EXPECT_CALL(model, insert_session(_)).Times(1);
    EXPECT_CALL(model, remove_session(_)).Times(1);

    EXPECT_CALL(mechanism, focus(_,_)).Times(1);
    EXPECT_CALL(mechanism, focus(_,std::shared_ptr<mf::ApplicationSession>())).Times(1);

    EXPECT_CALL(strategy, previous_focus_app(_)).WillOnce(Return((std::shared_ptr<mf::ApplicationSession>())));
    
    auto session = app_manager.open_session("Visual Basic Studio");

    auto surf = session->create_surface(ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    
    EXPECT_CALL(organiser, destroy_surface(_)).Times(1);

    app_manager.close_session(session);
}

TEST(ApplicationManager, new_applications_receive_focus)
{
    using namespace ::testing;
    ms::MockApplicationSurfaceOrganiser organiser;
    MockApplicationSessionModel model;
    MockFocusStrategy strategy;
    MockFocusMechanism mechanism;
    std::shared_ptr<mf::ApplicationSession> new_session;

    mf::ApplicationManager app_manager(std::shared_ptr<ms::ApplicationSurfaceOrganiser>(&organiser, mir::EmptyDeleter()), 
                                       std::shared_ptr<mf::ApplicationSessionContainer>(&model, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::ApplicationFocusStrategy>(&strategy, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::ApplicationFocusMechanism>(&mechanism, mir::EmptyDeleter()));
    
    EXPECT_CALL(model, insert_session(_)).Times(1);
    EXPECT_CALL(mechanism, focus(_,_)).WillOnce(SaveArg<1>(&new_session));

    auto session = app_manager.open_session("Visual Basic Studio");
    EXPECT_EQ(session, new_session);
}
