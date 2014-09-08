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

#include "src/server/scene/application_session.h"
#include "src/server/shell/default_focus_mechanism.h"
#include "mir/scene/session.h"
#include "mir/scene/surface_creation_parameters.h"
#include "src/server/scene/basic_surface.h"
#include "mir/graphics/display_configuration.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_buffer_stream.h"
#include "mir_test_doubles/mock_scene_session.h"
#include "mir_test_doubles/mock_surface.h"
#include "mir_test_doubles/mock_surface_coordinator.h"
#include "mir_test_doubles/stub_input_targeter.h"
#include "mir_test_doubles/mock_input_targeter.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(DefaultFocusMechanism, mechanism_notifies_default_surface_of_focus_changes)
{
    using namespace ::testing;

    NiceMock<mtd::MockSceneSession> app1, app2;
    auto const mock_surface1 = std::make_shared<NiceMock<mtd::MockSurface>>();
    auto const mock_surface2 = std::make_shared<NiceMock<mtd::MockSurface>>();
    auto const surface_coordinator = std::make_shared<mtd::MockSurfaceCoordinator>();

    ON_CALL(app1, default_surface()).WillByDefault(Return(mock_surface1));
    ON_CALL(app2, default_surface()).WillByDefault(Return(mock_surface2));

    msh::DefaultFocusMechanism focus_mechanism(
        std::make_shared<mtd::StubInputTargeter>(),
        surface_coordinator);

    InSequence seq;
    EXPECT_CALL(*surface_coordinator, raise(_)).Times(1);
    EXPECT_CALL(*mock_surface1, configure(mir_surface_attrib_focus, mir_surface_focused)).Times(1);
    EXPECT_CALL(*surface_coordinator, raise(_)).Times(1);
    EXPECT_CALL(*mock_surface1, configure(mir_surface_attrib_focus, mir_surface_unfocused)).Times(1);
    EXPECT_CALL(*mock_surface2, configure(mir_surface_attrib_focus, mir_surface_focused)).Times(1);

    focus_mechanism.set_focus_to(mt::fake_shared(app1));
    focus_mechanism.set_focus_to(mt::fake_shared(app2));
}

TEST(DefaultFocusMechanism, sets_input_focus)
{
    using namespace ::testing;

    NiceMock<mtd::MockSceneSession> app1;
    NiceMock<mtd::MockSurface> mock_surface;
    auto const surface_coordinator = std::make_shared<mtd::MockSurfaceCoordinator>();

    {
        InSequence seq;
        EXPECT_CALL(app1, default_surface()).Times(1)
            .WillOnce(Return(mt::fake_shared(mock_surface)));
        EXPECT_CALL(app1, default_surface()).Times(1)
            .WillOnce(Return(std::shared_ptr<ms::Surface>()));
    }

    NiceMock<mtd::MockInputTargeter> targeter;

    msh::DefaultFocusMechanism focus_mechanism(mt::fake_shared(targeter), surface_coordinator);

    {
        InSequence seq;
        EXPECT_CALL(mock_surface, take_input_focus(_)).Times(1);
        // When we have no default surface.
        EXPECT_CALL(targeter, focus_cleared()).Times(1);
        // When we have no session.
        EXPECT_CALL(targeter, focus_cleared()).Times(1);
    }
    EXPECT_CALL(*surface_coordinator, raise(_)).Times(1);

    focus_mechanism.set_focus_to(mt::fake_shared(app1));
    focus_mechanism.set_focus_to(mt::fake_shared(app1));
    focus_mechanism.set_focus_to(std::shared_ptr<ms::Session>());
}

TEST(DefaultFocusMechanism, notifies_surface_of_focus_change_after_it_has_taken_the_focus)
{
    using namespace ::testing;

    NiceMock<mtd::MockSceneSession> app;
    auto const mock_surface = std::make_shared<NiceMock<mtd::MockSurface>>();
    auto const surface_coordinator = std::make_shared<NiceMock<mtd::MockSurfaceCoordinator>>();

    ON_CALL(app, default_surface()).WillByDefault(Return(mock_surface));

    msh::DefaultFocusMechanism focus_mechanism(
        std::make_shared<mtd::StubInputTargeter>(),
        surface_coordinator);

    InSequence seq;
    EXPECT_CALL(*mock_surface, take_input_focus(_));
    EXPECT_CALL(*mock_surface, configure(mir_surface_attrib_focus, mir_surface_focused)).Times(1);

    focus_mechanism.set_focus_to(mt::fake_shared(app));
}
