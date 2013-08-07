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

#include "mir/surfaces/buffer_stream.h"
#include "mir/shell/focus_sequence.h"
#include "mir/shell/session_manager.h"
#include "mir/shell/default_session_container.h"
#include "mir/shell/session.h"
#include "mir/shell/surface.h"
#include "mir/shell/session_listener.h"
#include "mir/shell/null_session_listener.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/surfaces/surface.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_display_changer.h"
#include "mir_test_doubles/mock_session.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/mock_focus_setter.h"
#include "mir_test_doubles/mock_session_listener.h"
#include "mir_test_doubles/stub_surface_builder.h"
#include "mir_test_doubles/stub_surface_controller.h"
#include "mir_test_doubles/null_snapshot_strategy.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct MockSessionContainer : public msh::DefaultSessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<msh::Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<msh::Session> const&));
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
    ~MockSessionContainer() noexcept {}
};

struct SessionManagerSetup : public testing::Test
{
    SessionManagerSetup()
      : session_manager(mt::fake_shared(surface_factory),
                        mt::fake_shared(container),
                        mt::fake_shared(focus_setter),
                        std::make_shared<mtd::NullSnapshotStrategy>(),
                        mt::fake_shared(session_listener),
                        mt::fake_shared(mock_display_changer))
    {
    }

    mtd::StubSurfaceBuilder surface_builder;
    mtd::StubSurfaceController surface_controller;
    mtd::MockSurfaceFactory surface_factory;
    testing::NiceMock<MockSessionContainer> container;    // Inelegant but some tests need a stub
    testing::NiceMock<mtd::MockFocusSetter> focus_setter; // Inelegant but some tests need a stub
    mtd::MockSessionListener session_listener;
    testing::NiceMock<mtd::MockDisplayChanger> mock_display_changer;

    msh::SessionManager session_manager;
};

}

TEST_F(SessionManagerSetup, open_and_close_session)
{
    using namespace ::testing;

    Sequence seq;
    EXPECT_CALL(container, insert_session(_))
        .InSequence(seq);
    EXPECT_CALL(session_listener, starting(_))
        .InSequence(seq);
    EXPECT_CALL(focus_setter, session_opened(_))
        .InSequence(seq);

    EXPECT_CALL(container, remove_session(_))
        .InSequence(seq);
    EXPECT_CALL(session_listener, stopping(_))
        .InSequence(seq);
    EXPECT_CALL(focus_setter, session_closed(_))
        .InSequence(seq);

    auto session = session_manager.open_session("Visual Basic Studio", std::shared_ptr<mf::EventSink>());
    session_manager.close_session(session);
}

TEST_F(SessionManagerSetup, create_surface)
{
    using namespace testing;

    std::shared_ptr<mf::Session> session;
    EXPECT_CALL(focus_setter, surface_created_for(_))
        .Times(1);

    session_manager.handle_surface_created(session);
}

namespace
{
struct MockShellSession : public msh::Session
{
    MOCK_METHOD1(create_surface, mf::SurfaceId(msh::SurfaceCreationParameters const&));
    MOCK_METHOD1(destroy_surface, void(mf::SurfaceId));
    MOCK_CONST_METHOD1(get_surface, std::shared_ptr<mf::Surface>(mf::SurfaceId));

    MOCK_METHOD1(take_snapshot, void(msh::SnapshotCallback const&));
    MOCK_CONST_METHOD0(default_surface, std::shared_ptr<msh::Surface>());

    MOCK_CONST_METHOD0(name, std::string());
    MOCK_METHOD0(force_requests_to_complete, void());

    MOCK_METHOD0(hide, void());
    MOCK_METHOD0(show, void());
    
    MOCK_METHOD3(configure_surface, int(mf::SurfaceId, MirSurfaceAttrib, int));
};
}
TEST_F(SessionManagerSetup, display_change_configuration)
{
    using namespace testing;

    auto session1 = std::make_shared<MockShellSession>();
    auto session2 = std::make_shared<MockShellSession>();

    EXPECT_CALL(focus_setter, focused_session())
        .Times(2)
        .WillOnce(Return(session1))
        .WillOnce(Return(session2));
    EXPECT_CALL(mock_display_changer, apply_configuration_of(_))
        .Times(1);

    session_manager.handle_display_configuration(session2);
}
