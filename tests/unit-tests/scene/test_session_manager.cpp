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

#include "src/server/scene/session_manager.h"
#include "mir/compositor/buffer_stream.h"
#include "src/server/scene/default_session_container.h"
#include "mir/scene/surface.h"
#include "mir/scene/session.h"
#include "mir/scene/session_listener.h"
#include "mir/scene/null_session_listener.h"
#include "mir/scene/surface_creation_parameters.h"
#include "src/server/scene/session_event_sink.h"
#include "src/server/scene/basic_surface.h"
#include "src/server/report/null_report_factory.h"
#include "mir/scene/prompt_session_creation_parameters.h"
#include "mir/scene/prompt_session.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_surface_coordinator.h"
#include "mir_test_doubles/mock_session_listener.h"
#include "mir_test_doubles/stub_buffer_stream.h"
#include "mir_test_doubles/null_snapshot_strategy.h"
#include "mir_test_doubles/null_surface_configurator.h"
#include "mir_test_doubles/null_session_event_sink.h"
#include "mir_test_doubles/mock_prompt_session_listener.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/null_prompt_session_manager.h"
#include "mir_test_doubles/null_surface_configurator.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mi = mir::input;
namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{
struct MockSessionContainer : public ms::SessionContainer
{
    MOCK_METHOD1(insert_session, void(std::shared_ptr<ms::Session> const&));
    MOCK_METHOD1(remove_session, void(std::shared_ptr<ms::Session> const&));
    MOCK_CONST_METHOD1(successor_of, std::shared_ptr<ms::Session>(std::shared_ptr<ms::Session> const&));
    MOCK_CONST_METHOD1(for_each, void(std::function<void(std::shared_ptr<ms::Session> const&)>));
    MOCK_METHOD0(lock, void());
    MOCK_METHOD0(unlock, void());
    ~MockSessionContainer() noexcept {}
};

struct MockSessionEventSink : public ms::SessionEventSink
{
    MOCK_METHOD1(handle_focus_change, void(std::shared_ptr<ms::Session> const& session));
    MOCK_METHOD0(handle_no_focus, void());
    MOCK_METHOD1(handle_session_stopping, void(std::shared_ptr<ms::Session> const& session));
};

struct SessionManagerSetup : public testing::Test
{
    SessionManagerSetup()
      : session_manager(mt::fake_shared(surface_coordinator),
                        mt::fake_shared(container),
                        std::make_shared<mtd::NullSnapshotStrategy>(),
                        std::make_shared<mtd::NullSessionEventSink>(),
                        mt::fake_shared(session_listener),
                        std::make_shared<mtd::NullPromptSessionManager>())
    {
        using namespace ::testing;
        ON_CALL(container, successor_of(_)).WillByDefault(Return((std::shared_ptr<ms::Session>())));
    }

    mtd::NullSurfaceConfigurator surface_configurator;
    std::shared_ptr<ms::Surface> dummy_surface = std::make_shared<ms::BasicSurface>(
        std::string("stub"),
        geom::Rectangle{{},{}},
        false,
        std::make_shared<mtd::StubBufferStream>(),
        std::shared_ptr<mi::InputChannel>(),
        std::shared_ptr<mi::InputSender>(),
        mt::fake_shared<ms::SurfaceConfigurator>(surface_configurator),
        std::shared_ptr<mg::CursorImage>(),
        mir::report::null_scene_report());
    mtd::MockSurfaceCoordinator surface_coordinator;
    testing::NiceMock<MockSessionContainer> container;    // Inelegant but some tests need a stub
    ms::NullSessionListener session_listener;

    ms::SessionManager session_manager;
};

}

TEST_F(SessionManagerSetup, open_and_close_session)
{
    using namespace ::testing;

    EXPECT_CALL(container, insert_session(_)).Times(1);
    EXPECT_CALL(container, remove_session(_)).Times(1);

    auto session = session_manager.open_session(__LINE__, "Visual Basic Studio", std::shared_ptr<mf::EventSink>());
    session_manager.close_session(session);
}

TEST_F(SessionManagerSetup, closing_session_removes_surfaces)
{
    using namespace ::testing;

    EXPECT_CALL(surface_coordinator, add_surface(_, _)).Times(1);

    ON_CALL(surface_coordinator, add_surface(_, _)).WillByDefault(
       Return(dummy_surface));

    EXPECT_CALL(container, insert_session(_)).Times(1);
    EXPECT_CALL(container, remove_session(_)).Times(1);

    auto session = session_manager.open_session(__LINE__, "Visual Basic Studio", std::shared_ptr<mf::EventSink>());
    session->create_surface(ms::a_surface().of_size(geom::Size{geom::Width{1024}, geom::Height{768}}));

    session_manager.close_session(session);
}

namespace
{
struct SessionManagerSessionListenerSetup : public testing::Test
{
    SessionManagerSessionListenerSetup()
      : session_manager(mt::fake_shared(surface_coordinator),
                        mt::fake_shared(container),
                        std::make_shared<mtd::NullSnapshotStrategy>(),
                        std::make_shared<mtd::NullSessionEventSink>(),
                        mt::fake_shared(session_listener),
                        std::make_shared<mtd::NullPromptSessionManager>())
    {
        using namespace ::testing;
        ON_CALL(container, successor_of(_)).WillByDefault(Return((std::shared_ptr<ms::Session>())));
    }

    mtd::MockSurfaceCoordinator surface_coordinator;
    testing::NiceMock<MockSessionContainer> container;    // Inelegant but some tests need a stub
    testing::NiceMock<mtd::MockSessionListener> session_listener;

    ms::SessionManager session_manager;
};
}

TEST_F(SessionManagerSessionListenerSetup, session_listener_is_notified_of_lifecycle)
{
    using namespace ::testing;

    EXPECT_CALL(session_listener, starting(_)).Times(1);
    EXPECT_CALL(session_listener, stopping(_)).Times(1);

    auto session = session_manager.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    session_manager.close_session(session);
}

namespace
{
struct SessionManagerSessionEventsSetup : public testing::Test
{
    void SetUp() override
    {
        using namespace ::testing;
        ON_CALL(container, successor_of(_)).WillByDefault(Return((std::shared_ptr<ms::Session>())));
    }

    mtd::MockSurfaceCoordinator surface_coordinator;
    testing::NiceMock<MockSessionContainer> container;    // Inelegant but some tests need a stub
    MockSessionEventSink session_event_sink;
    testing::NiceMock<mtd::MockSessionListener> session_listener;

    ms::SessionManager session_manager{
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(container),
        std::make_shared<mtd::NullSnapshotStrategy>(),
        mt::fake_shared(session_event_sink),
        mt::fake_shared(session_listener),
        std::make_shared<mtd::NullPromptSessionManager>()};
};
}

TEST_F(SessionManagerSessionEventsSetup, session_event_sink_is_notified_of_lifecycle)
{
    using namespace ::testing;

    auto session = session_manager.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = session_manager.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());

    Mock::VerifyAndClearExpectations(&session_event_sink);

    EXPECT_CALL(session_event_sink, handle_focus_change(_)).Times(AnyNumber());
    EXPECT_CALL(session_event_sink, handle_no_focus()).Times(AnyNumber());

    InSequence s;
    EXPECT_CALL(session_event_sink, handle_session_stopping(_)).Times(1);
    EXPECT_CALL(session_event_sink, handle_session_stopping(_)).Times(1);

    session_manager.close_session(session1);
    session_manager.close_session(session);
}

// TODO the following tests replace unit tests of "window management" functions
// TODO that were implemented by SessionManager.
// TODO They are now actually integration tests of DefaultShell + SessionManager
// TODO but as I'm reworking that interaction and they use mocks that only exist
// TODO in this file (e.g. MockSessionContainer) I've left them here temporarily.
#include "src/server/shell/default_shell.h"
#include "src/server/shell/default_focus_mechanism.h"
#include "mir_test_doubles/mock_focus_setter.h"

namespace msh = mir::shell;
using namespace ::testing;

namespace
{
struct DefaultShell : Test
{
    mtd::MockSurfaceCoordinator surface_coordinator;
    NiceMock<MockSessionContainer> container;
    NiceMock<MockSessionEventSink> session_event_sink;
    NiceMock<mtd::MockSessionListener> session_listener;

    ms::SessionManager session_manager{
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(container),
        std::make_shared<mtd::NullSnapshotStrategy>(),
        mt::fake_shared(session_event_sink),
        mt::fake_shared(session_listener),
        std::make_shared<mtd::NullPromptSessionManager>()};

    NiceMock<mtd::MockFocusSetter> focus_setter;
    msh::DefaultShell shell{mt::fake_shared(focus_setter), mt::fake_shared(session_manager)};

    void SetUp() override
    {
        ON_CALL(container, successor_of(_)).WillByDefault(Return((std::shared_ptr<ms::Session>())));
    }
};
}

TEST_F(DefaultShell, new_applications_receive_focus)
{
    using namespace ::testing;
    std::shared_ptr<ms::Session> new_session;

    EXPECT_CALL(container, insert_session(_)).Times(1);
    EXPECT_CALL(focus_setter, set_focus_to(_)).WillOnce(SaveArg<0>(&new_session));

    auto session = shell.open_session(__LINE__, "Visual Basic Studio", std::shared_ptr<mf::EventSink>());
    EXPECT_EQ(session, new_session);
}

TEST_F(DefaultShell, session_listener_is_notified_of_focus)
{
    using namespace ::testing;

    EXPECT_CALL(session_listener, focused(_)).Times(1);
    EXPECT_CALL(session_listener, unfocused()).Times(1);

    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    shell.close_session(session);
}

TEST_F(DefaultShell, session_event_sink_is_notified_of_focus)
{
    using namespace ::testing;

    EXPECT_CALL(session_event_sink, handle_focus_change(_)).Times(2);

    auto session = shell.open_session(__LINE__, "XPlane", std::shared_ptr<mf::EventSink>());
    auto session1 = shell.open_session(__LINE__, "Bla", std::shared_ptr<mf::EventSink>());

    Mock::VerifyAndClearExpectations(&session_event_sink);

    InSequence s;
    EXPECT_CALL(session_event_sink, handle_session_stopping(_)).Times(1);
    EXPECT_CALL(container, successor_of(_)).
        WillOnce(Return(std::dynamic_pointer_cast<ms::Session>(session)));
    EXPECT_CALL(session_event_sink, handle_focus_change(_)).Times(1);
    EXPECT_CALL(session_event_sink, handle_session_stopping(_)).Times(1);
    EXPECT_CALL(container, successor_of(_)).
        WillOnce(Return(std::shared_ptr<ms::Session>()));
    EXPECT_CALL(session_event_sink, handle_no_focus()).Times(1);

    shell.close_session(session1);
    shell.close_session(session);
}
