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
#include "mir/frontend/session_manager.h"
#include "mir/frontend/session_container.h"
#include "mir/frontend/session.h"
#include "mir/frontend/focus_sequence.h"
#include "mir/frontend/focus_setter.h"
#include "mir/surfaces/surface.h"
#include "mir_test_doubles/mock_buffer_bundle.h"
#include "mir_test/empty_deleter.h"
#include "mir_test_doubles/mock_surface_organiser.h"
#include "null_buffer_bundle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

struct MockSessionContainer : public mf::SessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<mf::Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<mf::Session> const&));
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
};

struct MockFocusSequence: public mf::FocusSequence
{
    MOCK_CONST_METHOD1(successor_of, std::weak_ptr<mf::Session>(std::shared_ptr<mf::Session> const&));
    MOCK_CONST_METHOD1(predecessor_of, std::weak_ptr<mf::Session>(std::shared_ptr<mf::Session> const&));
};

struct MockFocusSetter: public mf::FocusSetter
{
    MOCK_METHOD1(set_focus_to, void(std::shared_ptr<mf::Session> const&));
};

}

TEST(SessionManager, open_and_close_session)
{
    using namespace ::testing;
    mtd::MockSurfaceOrganiser organiser;
    MockSessionContainer container;
    MockFocusSequence sequence;
    MockFocusSetter focus_setter;

    mf::SessionManager session_manager(std::shared_ptr<mf::SurfaceOrganiser>(&organiser, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::SessionContainer>(&container, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::FocusSequence>(&sequence, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::FocusSetter>(&focus_setter, mir::EmptyDeleter()));

    EXPECT_CALL(container, insert_session(_)).Times(1);
    EXPECT_CALL(container, remove_session(_)).Times(1);
    EXPECT_CALL(focus_setter, set_focus_to(_));
    EXPECT_CALL(focus_setter, set_focus_to(std::shared_ptr<mf::Session>())).Times(1);

    EXPECT_CALL(sequence, predecessor_of(_)).WillOnce(Return((std::shared_ptr<mf::Session>())));

    auto session = session_manager.open_session("Visual Basic Studio");
    session_manager.close_session(session);
}

TEST(SessionManager, closing_session_removes_surfaces)
{
    using namespace ::testing;
    mtd::MockSurfaceOrganiser organiser;
    MockSessionContainer container;
    MockFocusSequence sequence;
    MockFocusSetter mechanism;

    mf::SessionManager session_manager(std::shared_ptr<mf::SurfaceOrganiser>(&organiser, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::SessionContainer>(&container, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::FocusSequence>(&sequence, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::FocusSetter>(&mechanism, mir::EmptyDeleter()));

    EXPECT_CALL(organiser, create_surface(_)).Times(1);
    std::shared_ptr<mc::BufferBundle> buffer_bundle(new mt::NullBufferBundle());
    std::shared_ptr<ms::Surface> dummy_surface(
        new ms::Surface(
            ms::a_surface().name,
            buffer_bundle));
    ON_CALL(organiser, create_surface(_)).WillByDefault(Return(dummy_surface));
    EXPECT_CALL(organiser, destroy_surface(_)).Times(1);

    EXPECT_CALL(container, insert_session(_)).Times(1);
    EXPECT_CALL(container, remove_session(_)).Times(1);

    EXPECT_CALL(mechanism, set_focus_to(_)).Times(1);
    EXPECT_CALL(mechanism, set_focus_to(std::shared_ptr<mf::Session>())).Times(1);

    EXPECT_CALL(sequence, predecessor_of(_)).WillOnce(Return((std::shared_ptr<mf::Session>())));

    auto session = session_manager.open_session("Visual Basic Studio");
    session->create_surface(ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    session_manager.close_session(session);
}

TEST(SessionManager, new_applications_receive_focus)
{
    using namespace ::testing;
    mtd::MockSurfaceOrganiser organiser;
    MockSessionContainer container;
    MockFocusSequence sequence;
    MockFocusSetter mechanism;
    std::shared_ptr<mf::Session> new_session;

    mf::SessionManager session_manager(std::shared_ptr<mf::SurfaceOrganiser>(&organiser, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::SessionContainer>(&container, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::FocusSequence>(&sequence, mir::EmptyDeleter()),
                                       std::shared_ptr<mf::FocusSetter>(&mechanism, mir::EmptyDeleter()));

    EXPECT_CALL(container, insert_session(_)).Times(1);
    EXPECT_CALL(mechanism, set_focus_to(_)).WillOnce(SaveArg<0>(&new_session));

    auto session = session_manager.open_session("Visual Basic Studio");
    EXPECT_EQ(session, new_session);
}
