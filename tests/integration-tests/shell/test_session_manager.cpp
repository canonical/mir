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
#include "mir/shell/shell_configuration.h"
#include "mir/shell/session.h"
#include "mir/shell/focus_sequence.h"
#include "mir/shell/focus_setter.h"
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/session_container.h"
#include "mir/surfaces/buffer_bundle.h"
#include "mir/surfaces/surface.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/frontend/surface_creation_parameters.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/mock_focus_setter.h"
#include "mir_test_doubles/stub_input_target_listener.h"

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

struct TestingShellConfiguration : public msh::ShellConfiguration
{
    TestingShellConfiguration() :
        sequence(mt::fake_shared(container))
    {
    }
    ~TestingShellConfiguration() noexcept(true) {}

    std::shared_ptr<msh::SurfaceFactory> the_surface_factory()
    {
        return mt::fake_shared(surface_factory);
    }
    std::shared_ptr<msh::SessionContainer> the_session_container()
    {
        return mt::fake_shared(container);
    }
    std::shared_ptr<msh::FocusSequence> the_focus_sequence()
    {
        return mt::fake_shared(sequence);
    }
    std::shared_ptr<msh::FocusSetter> the_focus_setter()
    {
        return mt::fake_shared(focus_setter);
    }
    std::shared_ptr<msh::InputTargetListener> the_input_target_listener()
    {
        return mt::fake_shared(input_listener);
    }

    mtd::MockSurfaceFactory surface_factory;
    msh::SessionContainer container;
    msh::RegistrationOrderFocusSequence sequence;
    mtd::MockFocusSetter focus_setter;
    mtd::StubInputTargetListener input_listener;
};

TEST(TestSessionManagerAndFocusSelectionStrategy, cycle_focus)
{
    using namespace ::testing;

    TestingShellConfiguration config;
    msh::SessionManager session_manager(mt::fake_shared(config));
    
    EXPECT_CALL(config.focus_setter, set_focus_to(_)).Times(3);

    auto session1 = session_manager.open_session("Visual Basic Studio");
    auto session2 = session_manager.open_session("Microsoft Access");
    auto session3 = session_manager.open_session("WordPerfect");

    {
      InSequence seq;
      EXPECT_CALL(config.focus_setter, set_focus_to(Eq(session1))).Times(1);
      EXPECT_CALL(config.focus_setter, set_focus_to(Eq(session2))).Times(1);
      EXPECT_CALL(config.focus_setter, set_focus_to(Eq(session3))).Times(1);
    }

    session_manager.focus_next();
    session_manager.focus_next();
    session_manager.focus_next();
}

TEST(TestSessionManagerAndFocusSelectionStrategy, closing_applications_transfers_focus)
{
    using namespace ::testing;

    TestingShellConfiguration config;
    msh::SessionManager session_manager(mt::fake_shared(config));

    EXPECT_CALL(config.focus_setter, set_focus_to(_)).Times(3);

    auto session1 = session_manager.open_session("Visual Basic Studio");
    auto session2 = session_manager.open_session("Microsoft Access");
    auto session3 = session_manager.open_session("WordPerfect");

    {
      InSequence seq;
      EXPECT_CALL(config.focus_setter, set_focus_to(Eq(session2))).Times(1);
      EXPECT_CALL(config.focus_setter, set_focus_to(Eq(session1))).Times(1);
    }

    session_manager.close_session(session3);
    session_manager.close_session(session2);
}
