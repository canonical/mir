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

#include "mir/shell/abstract_shell.h"
#include "mir/scene/session.h"
#include "mir/scene/null_session_listener.h"
#include "mir/scene/placement_strategy.h"

#include "src/server/shell/default_window_manager.h"
#include "src/server/scene/session_manager.h"
#include "src/server/scene/default_session_container.h"

#include "mir_test/gmock_fixes.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/stub_scene_session.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/mock_surface_coordinator.h"
#include "mir_test_doubles/null_snapshot_strategy.h"
#include "mir_test_doubles/null_event_sink.h"
#include "mir_test_doubles/null_session_event_sink.h"
#include "mir_test_doubles/mock_surface_configurator.h"
#include "mir_test_doubles/null_prompt_session_manager.h"
#include "mir_test_doubles/mock_input_targeter.h"
#include "mir_test_doubles/stub_buffer_stream_factory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

using namespace ::testing;

namespace
{
struct MockSessionManager : ms::SessionManager
{
    using ms::SessionManager::SessionManager;

    MOCK_METHOD1(set_focus_to, void (std::shared_ptr<ms::Session> const& focus));
};

struct NullPlacementStrategy : ms::PlacementStrategy
{
    auto place(ms::Session const& , ms::SurfaceCreationParameters const& params)
    -> ms::SurfaceCreationParameters  override { return params; }
};

struct TestDefaultWindowManager : public testing::Test
{
    NiceMock<mtd::MockSurfaceCoordinator> surface_coordinator;
    ms::DefaultSessionContainer container;
    NiceMock<mtd::MockInputTargeter> input_targeter;
    std::shared_ptr<mf::Session> new_session;
    ms::NullSessionListener session_listener;
    mtd::StubBufferStreamFactory buffer_stream_factory;
    NiceMock<MockSessionManager> session_manager
        {
            mt::fake_shared(surface_coordinator),
            mt::fake_shared(buffer_stream_factory),
            mt::fake_shared(container),
            std::make_shared<mtd::NullSnapshotStrategy>(),
            std::make_shared<mtd::NullSessionEventSink>(),
            mt::fake_shared(session_listener)
        };

    NiceMock<mtd::MockSurfaceConfigurator> surface_configurator;

    msh::AbstractShell shell{
        mt::fake_shared(input_targeter),
        mt::fake_shared(surface_coordinator),
        mt::fake_shared(session_manager),
        std::make_shared<mtd::NullPromptSessionManager>(),
        [this](msh::FocusController* focus_controller)
            { return std::make_shared<msh::DefaultWindowManager>(
                focus_controller,
                std::make_shared<NullPlacementStrategy>(),
                mt::fake_shared(session_manager),
                mt::fake_shared(surface_configurator));
            }};
};
}

TEST_F(TestDefaultWindowManager, cycle_focus)
{
    EXPECT_CALL(session_manager, set_focus_to(_)).Times(3);

    auto session1 = shell.open_session(__LINE__, "Visual Basic Studio", std::make_shared<mtd::NullEventSink>());
    auto session2 = shell.open_session(__LINE__, "Microsoft Access", std::make_shared<mtd::NullEventSink>());
    auto session3 = shell.open_session(__LINE__, "WordPerfect", std::make_shared<mtd::NullEventSink>());

    Mock::VerifyAndClearExpectations(&session_manager);

    {
      InSequence seq;
      EXPECT_CALL(session_manager, set_focus_to(Eq(session1))).Times(1);
      EXPECT_CALL(session_manager, set_focus_to(Eq(session2))).Times(1);
      EXPECT_CALL(session_manager, set_focus_to(Eq(session3))).Times(1);
    }

    shell.focus_next();
    shell.focus_next();
    shell.focus_next();

    Mock::VerifyAndClearExpectations(&session_manager);

    // Possible change of focus while sessions are closed on shutdown
    EXPECT_CALL(session_manager, set_focus_to(_)).Times(AnyNumber());
}

TEST_F(TestDefaultWindowManager, closing_applications_transfers_focus)
{
    using namespace ::testing;

    EXPECT_CALL(session_manager, set_focus_to(_)).Times(3);

    auto session1 = shell.open_session(__LINE__, "Visual Basic Studio", std::make_shared<mtd::NullEventSink>());
    auto session2 = shell.open_session(__LINE__, "Microsoft Access", std::make_shared<mtd::NullEventSink>());
    auto session3 = shell.open_session(__LINE__, "WordPerfect", std::make_shared<mtd::NullEventSink>());

    Mock::VerifyAndClearExpectations(&session_manager);

    {
      InSequence seq;
      EXPECT_CALL(session_manager, set_focus_to(Eq(session2))).Times(1);
      EXPECT_CALL(session_manager, set_focus_to(Eq(session1))).Times(1);
    }

    shell.close_session(session3);
    shell.close_session(session2);

    Mock::VerifyAndClearExpectations(&session_manager);

    // Possible change of focus while sessions are closed on shutdown
    EXPECT_CALL(session_manager, set_focus_to(_)).Times(AtLeast(0));
}

TEST_F(TestDefaultWindowManager, sets_input_focus)
{
    mtd::StubSceneSession app1;
    NiceMock<mtd::MockSurface> mock_surface;

    {
        InSequence seq;
        EXPECT_CALL(mock_surface, take_input_focus(_)).Times(1);
        // When we have no default surface.
        EXPECT_CALL(input_targeter, focus_cleared()).Times(1);
        // When we have no session.
        EXPECT_CALL(input_targeter, focus_cleared()).Times(1);
    }

    shell.set_focus_to(mt::fake_shared(app1), mt::fake_shared(mock_surface));
    shell.set_focus_to(mt::fake_shared(app1), std::shared_ptr<ms::Surface>());
    shell.set_focus_to(std::shared_ptr<ms::Session>(), std::shared_ptr<ms::Surface>());
}

TEST_F(TestDefaultWindowManager, notifies_surface_of_focus_change_after_it_has_taken_the_focus)
{
    mtd::StubSceneSession app;
    auto const mock_surface = std::make_shared<NiceMock<mtd::MockSurface>>();

    InSequence seq;
    EXPECT_CALL(*mock_surface, take_input_focus(_));
    EXPECT_CALL(*mock_surface, configure(mir_surface_attrib_focus, mir_surface_focused)).Times(1);

    shell.set_focus_to(mt::fake_shared(app), mock_surface);
}

TEST_F(TestDefaultWindowManager, configurator_selects_attribute_values)
{
    mtd::StubSceneSession app;
    auto const session = mt::fake_shared(app);
    auto const surface = std::make_shared<NiceMock<mtd::MockSurface>>();
    ON_CALL(*surface, configure(_, _)).WillByDefault(ReturnArg<1>());

    InSequence seq;

    EXPECT_CALL(surface_configurator, select_attribute_value(_, mir_surface_attrib_state, mir_surface_state_restored))
        .WillOnce(Return(mir_surface_state_minimized));

    EXPECT_CALL(*surface, configure(mir_surface_attrib_state, mir_surface_state_minimized));

    EXPECT_CALL(surface_configurator, attribute_set(_, mir_surface_attrib_state, mir_surface_state_minimized));

    EXPECT_THAT(
        shell.set_surface_attribute(session, surface, mir_surface_attrib_state, mir_surface_state_restored),
        Eq(mir_surface_state_minimized));
}

TEST_F(TestDefaultWindowManager, set_surface_attribute_returns_value_set_by_configurator)
{
    mtd::StubSceneSession app;
    auto const session = mt::fake_shared(app);
    auto const surface = std::make_shared<NiceMock<mtd::MockSurface>>();
    ON_CALL(*surface, configure(_, _)).WillByDefault(ReturnArg<1>());

    ON_CALL(surface_configurator, select_attribute_value(_, Not(mir_surface_attrib_focus), _))
        .WillByDefault(ReturnArg<1>());

    ON_CALL(surface_configurator, select_attribute_value(_, mir_surface_attrib_focus, mir_surface_focused))
        .WillByDefault(Return(mir_surface_unfocused));

    ON_CALL(surface_configurator, select_attribute_value(_, mir_surface_attrib_focus, Not(mir_surface_focused)))
        .WillByDefault(Return(mir_surface_focused));

    EXPECT_THAT(
        shell.set_surface_attribute(session, surface, mir_surface_attrib_focus, mir_surface_focused),
        Eq(mir_surface_unfocused));

    EXPECT_THAT(
        shell.set_surface_attribute(session, surface, mir_surface_attrib_focus, mir_surface_unfocused),
        Eq(mir_surface_focused));
}
