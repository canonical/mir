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
#include "mir/frontend/application_focus_selection_strategy.h"
#include "mir/frontend/focus.h"
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

struct MockApplicationSessionModel : public mf::SessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<mf::Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<mf::Session> const&));
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
    MOCK_METHOD0(iterator, std::shared_ptr<mf::SessionContainer::LockingIterator>());
};

struct MockFocusSelectionStrategy: public mf::FocusSequence
{
    MOCK_METHOD1(successor_of, std::weak_ptr<mf::Session>(std::shared_ptr<mf::Session> const&));
    MOCK_METHOD1(predecessor_of, std::weak_ptr<mf::Session>(std::shared_ptr<mf::Session> const&));
};
  
struct MockFocus: public mf::Focus
{
    MOCK_METHOD1(set_focus_to, void(std::shared_ptr<mf::Session> const&));
};

}

TEST(SessionManager, open_and_close_session)
{
    using namespace ::testing;
    mf::MockSurfaceOrganiser organiser;
    MockApplicationSessionModel model;
    MockFocusSelectionStrategy strategy;
    MockFocus focus;

    mf::SessionManager app_manager(std::shared_ptr<mf::SurfaceOrganiser>(&organiser, mir::EmptyDeleter()), 
                                       std::shared_ptr<mf::SessionContainer>(&model, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::FocusSequence>(&strategy, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::Focus>(&focus, mir::EmptyDeleter()));
    
    EXPECT_CALL(model, insert_session(_)).Times(1);
    EXPECT_CALL(model, remove_session(_)).Times(1);
    EXPECT_CALL(focus, set_focus_to(_));
    EXPECT_CALL(focus, set_focus_to(std::shared_ptr<mf::Session>())).Times(1);

    EXPECT_CALL(strategy, predecessor_of(_)).WillOnce(Return((std::shared_ptr<mf::Session>())));

    auto session = app_manager.open_session("Visual Basic Studio");
    app_manager.close_session(session);
}

TEST(SessionManager, closing_session_removes_surfaces)
{
    using namespace ::testing;
    mf::MockSurfaceOrganiser organiser;
    MockApplicationSessionModel model;
    MockFocusSelectionStrategy strategy;
    MockFocus mechanism;

    mf::SessionManager app_manager(std::shared_ptr<mf::SurfaceOrganiser>(&organiser, mir::EmptyDeleter()), 
                                       std::shared_ptr<mf::SessionContainer>(&model, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::FocusSequence>(&strategy, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::Focus>(&mechanism, mir::EmptyDeleter()));
    
    EXPECT_CALL(organiser, create_surface(_)).Times(1);
    std::shared_ptr<mc::BufferBundle> buffer_bundle(
        new mc::MockBufferBundle());
    std::shared_ptr<ms::Surface> dummy_surface(
        new ms::Surface(
            ms::a_surface().name,
            buffer_bundle));
    ON_CALL(organiser, create_surface(_)).WillByDefault(Return(dummy_surface));
    EXPECT_CALL(organiser, destroy_surface(_)).Times(1);

    EXPECT_CALL(model, insert_session(_)).Times(1);
    EXPECT_CALL(model, remove_session(_)).Times(1);

    EXPECT_CALL(mechanism, set_focus_to(_)).Times(1);
    EXPECT_CALL(mechanism, set_focus_to(std::shared_ptr<mf::Session>())).Times(1);

    EXPECT_CALL(strategy, predecessor_of(_)).WillOnce(Return((std::shared_ptr<mf::Session>())));
    
    auto session = app_manager.open_session("Visual Basic Studio");
    session->create_surface(ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));
    
    app_manager.close_session(session);
}

TEST(SessionManager, new_applications_receive_focus)
{
    using namespace ::testing;
    mf::MockSurfaceOrganiser organiser;
    MockApplicationSessionModel model;
    MockFocusSelectionStrategy strategy;
    MockFocus mechanism;
    std::shared_ptr<mf::Session> new_session;

    mf::SessionManager app_manager(std::shared_ptr<mf::SurfaceOrganiser>(&organiser, mir::EmptyDeleter()), 
                                       std::shared_ptr<mf::SessionContainer>(&model, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::FocusSequence>(&strategy, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::Focus>(&mechanism, mir::EmptyDeleter()));
    
    EXPECT_CALL(model, insert_session(_)).Times(1);
    EXPECT_CALL(mechanism, set_focus_to(_)).WillOnce(SaveArg<0>(&new_session));

    auto session = app_manager.open_session("Visual Basic Studio");
    EXPECT_EQ(session, new_session);
}
