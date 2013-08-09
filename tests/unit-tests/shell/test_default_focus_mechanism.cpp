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
 * Authored By: Robert Carr <racarr@canonical.com>
 */

#include "mir/shell/application_session.h"
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/default_focus_mechanism.h"
#include "mir/shell/session.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/surfaces/surface.h"
#include "mir/graphics/display_configuration.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/mock_shell_session.h"
#include "mir_test_doubles/stub_surface.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/stub_surface_builder.h"
#include "mir_test_doubles/stub_surface_controller.h"
#include "mir_test_doubles/stub_input_targeter.h"
#include "mir_test_doubles/mock_input_targeter.h"
#include "mir_test_doubles/stub_surface_controller.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(DefaultFocusMechanism, raises_default_surface)
{
    using namespace ::testing;
    
    NiceMock<mtd::MockShellSession> app1;
    mtd::MockSurface mock_surface(&app1, std::make_shared<mtd::StubSurfaceBuilder>());
    {
        InSequence seq;
        EXPECT_CALL(app1, default_surface()).Times(1)
            .WillOnce(Return(mt::fake_shared(mock_surface)));
    }

    auto controller = std::make_shared<mtd::StubSurfaceController>();
    EXPECT_CALL(mock_surface, raise(Eq(controller))).Times(1);
    mtd::StubInputTargeter targeter;
    msh::DefaultFocusMechanism focus_mechanism(mt::fake_shared(targeter), controller);
    
    focus_mechanism.set_focus_to(mt::fake_shared(app1));
}

TEST(DefaultFocusMechanism, sets_input_focus)
{
    using namespace ::testing;
    
    NiceMock<mtd::MockShellSession> app1;
    mtd::MockSurface mock_surface(&app1, std::make_shared<mtd::StubSurfaceBuilder>());
    {
        InSequence seq;
        EXPECT_CALL(app1, default_surface()).Times(1)
            .WillOnce(Return(mt::fake_shared(mock_surface)));
        EXPECT_CALL(app1, default_surface()).Times(1)
            .WillOnce(Return(std::shared_ptr<msh::Surface>()));
    }

    mtd::MockInputTargeter targeter;
    
    msh::DefaultFocusMechanism focus_mechanism(mt::fake_shared(targeter), std::make_shared<mtd::StubSurfaceController>());
    
    {
        InSequence seq;
        EXPECT_CALL(mock_surface, take_input_focus(_)).Times(1);
        // When we have no default surface.
        EXPECT_CALL(targeter, focus_cleared()).Times(1);
        // When we have no session.
        EXPECT_CALL(targeter, focus_cleared()).Times(1);
    }
    
    focus_mechanism.set_focus_to(mt::fake_shared(app1));
    focus_mechanism.set_focus_to(mt::fake_shared(app1));
    focus_mechanism.set_focus_to(std::shared_ptr<msh::Session>());
}
