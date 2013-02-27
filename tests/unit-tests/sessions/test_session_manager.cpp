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

#include "mir/surfaces/buffer_bundle.h"
#include "mir/sessions/session_manager.h"
#include "mir/sessions/session_container.h"
#include "mir/sessions/session.h"
#include "mir/sessions/surface_creation_parameters.h"
#include "mir/sessions/focus_sequence.h"
#include "mir/sessions/focus_setter.h"
#include "mir/surfaces/surface.h"
#include "mir_test_doubles/mock_buffer_bundle.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/null_buffer_bundle.h"

#include "src/surfaces/proxy_surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace msess = mir::sessions;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

struct MockSessionContainer : public msess::SessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<msess::Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<msess::Session> const&));
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
};

struct MockFocusSequence: public msess::FocusSequence
{
    MOCK_CONST_METHOD1(successor_of, std::shared_ptr<msess::Session>(std::shared_ptr<msess::Session> const&));
    MOCK_CONST_METHOD1(predecessor_of, std::shared_ptr<msess::Session>(std::shared_ptr<msess::Session> const&));
    MOCK_CONST_METHOD0(default_focus, std::shared_ptr<msess::Session>());
};

struct MockFocusSetter: public msess::FocusSetter
{
    MOCK_METHOD1(set_focus_to, void(std::shared_ptr<msess::Session> const&));
};

}

TEST(SessionManager, open_and_close_session)
{
    using namespace ::testing;
    mtd::MockSurfaceFactory surface_factory;
    MockSessionContainer container;
    MockFocusSequence sequence;
    MockFocusSetter focus_setter;

    msess::SessionManager session_manager(
        mt::fake_shared(surface_factory),
        mt::fake_shared(container),
        mt::fake_shared(sequence),
        mt::fake_shared(focus_setter));

    EXPECT_CALL(container, insert_session(_)).Times(1);
    EXPECT_CALL(container, remove_session(_)).Times(1);
    EXPECT_CALL(focus_setter, set_focus_to(_));
    EXPECT_CALL(focus_setter, set_focus_to(std::shared_ptr<msess::Session>())).Times(1);

    EXPECT_CALL(sequence, default_focus()).WillOnce(Return((std::shared_ptr<msess::Session>())));

    auto session = session_manager.open_session("Visual Basic Studio");
    session_manager.close_session(session);
}

TEST(SessionManager, closing_session_removes_surfaces)
{
    using namespace ::testing;
    mtd::MockSurfaceFactory surface_factory;
    MockSessionContainer container;
    MockFocusSequence sequence;
    MockFocusSetter mechanism;

    msess::SessionManager session_manager(
        mt::fake_shared(surface_factory),
        mt::fake_shared(container),
        mt::fake_shared(sequence),
        mt::fake_shared(mechanism));

    EXPECT_CALL(surface_factory, create_surface(_)).Times(1);
    std::shared_ptr<ms::BufferBundle> buffer_bundle(new mtd::NullBufferBundle());
    std::shared_ptr<ms::Surface> dummy_surface(
        std::make_shared<ms::Surface>(
            msess::a_surface().name,
            buffer_bundle));
    ON_CALL(surface_factory, create_surface(_)).WillByDefault(
        Return(std::make_shared<ms::BasicProxySurface>(dummy_surface)));

    EXPECT_CALL(container, insert_session(_)).Times(1);
    EXPECT_CALL(container, remove_session(_)).Times(1);

    EXPECT_CALL(mechanism, set_focus_to(_)).Times(1);
    EXPECT_CALL(mechanism, set_focus_to(std::shared_ptr<msess::Session>())).Times(1);

    EXPECT_CALL(sequence, default_focus()).WillOnce(Return((std::shared_ptr<msess::Session>())));

    auto session = session_manager.open_session("Visual Basic Studio");
    session->create_surface(msess::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    session_manager.close_session(session);
}

TEST(SessionManager, new_applications_receive_focus)
{
    using namespace ::testing;
    mtd::MockSurfaceFactory surface_factory;
    MockSessionContainer container;
    MockFocusSequence sequence;
    MockFocusSetter mechanism;
    std::shared_ptr<msess::Session> new_session;

    msess::SessionManager session_manager(
        mt::fake_shared(surface_factory),
        mt::fake_shared(container),
        mt::fake_shared(sequence),
        mt::fake_shared(mechanism));

    EXPECT_CALL(container, insert_session(_)).Times(1);
    EXPECT_CALL(mechanism, set_focus_to(_)).WillOnce(SaveArg<0>(&new_session));

    auto session = session_manager.open_session("Visual Basic Studio");
    EXPECT_EQ(session, new_session);
}

TEST(SessionManager, apps_selected_by_id_receive_focus)
{
    using namespace ::testing;
    mtd::MockSurfaceFactory surface_factory;
    NiceMock<MockSessionContainer> container;
    MockFocusSequence sequence;
    NiceMock<MockFocusSetter> mechanism;

    msess::SessionManager session_manager(
        mt::fake_shared(surface_factory),
        mt::fake_shared(container),
        mt::fake_shared(sequence),
        mt::fake_shared(mechanism));

    auto session1 = session_manager.open_session("Visual Basic Studio");
    auto session2 = session_manager.open_session("IntelliJ IDEA");

    session_manager.tag_session_with_lightdm_id(session1, 1);

    EXPECT_CALL(mechanism, set_focus_to(session1));
    session_manager.focus_session_with_lightdm_id(1);
}

TEST(SessionManager, closing_apps_selected_by_id_changes_focus)
{
    using namespace ::testing;
    mtd::MockSurfaceFactory surface_factory;
    NiceMock<MockSessionContainer> container;
    MockFocusSequence sequence;
    NiceMock<MockFocusSetter> mechanism;

    msess::SessionManager session_manager(
        mt::fake_shared(surface_factory),
        mt::fake_shared(container),
        mt::fake_shared(sequence),
        mt::fake_shared(mechanism));

    auto session1 = session_manager.open_session("Visual Basic Studio");
    auto session2 = session_manager.open_session("IntelliJ IDEA");

    session_manager.tag_session_with_lightdm_id(session1, 1);
    session_manager.focus_session_with_lightdm_id(1);

    EXPECT_CALL(sequence, default_focus()).WillOnce(Return(session2));
    EXPECT_CALL(mechanism, set_focus_to(session2));

    session_manager.close_session(session1);
}
