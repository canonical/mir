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

#include "mir/surfaces/buffer_bundle.h"
#include "mir/shell/application_session.h"
#include "mir/shell/session_container.h"
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/single_visibility_focus_mechanism.h"
#include "mir/frontend/surface_creation_parameters.h"
#include "mir/surfaces/surface.h"
#include "mir_test_doubles/mock_buffer_bundle.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/mock_session.h"
#include "mir_test_doubles/mock_input_focus_selector.h"
#include "mir_test_doubles/stub_surface.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(SingleVisibilityFocusMechanism, mechanism_sets_visibility)
{
    using namespace ::testing;

    NiceMock<mtd::MockInputFocusSelector> input_focus_selector;

    mtd::MockSession app1, app2, app3;
    msh::SessionContainer model;
    
    ON_CALL(app1, default_surface()).WillByDefault(Return(std::shared_ptr<mf::Surface>()));
    ON_CALL(app2, default_surface()).WillByDefault(Return(std::shared_ptr<mf::Surface>()));
    ON_CALL(app3, default_surface()).WillByDefault(Return(std::shared_ptr<mf::Surface>()));

    msh::SingleVisibilityFocusMechanism focus_mechanism(mt::fake_shared(model), mt::fake_shared(input_focus_selector));

    EXPECT_CALL(app1, show()).Times(1);
    EXPECT_CALL(app2, hide()).Times(1);
    EXPECT_CALL(app3, hide()).Times(1);

    model.insert_session(mt::fake_shared(app1));
    model.insert_session(mt::fake_shared(app2));
    model.insert_session(mt::fake_shared(app3));

    focus_mechanism.set_focus_to(mt::fake_shared(app1));
}

TEST(SingleVisibilityFocusMechanism, mechanism_sets_input_focus_from_default_surface)
{
    using namespace ::testing;

    mtd::MockInputFocusSelector input_focus_selector;
    msh::SessionContainer model;
    auto session = std::make_shared<mtd::MockSession>();
    auto surface = std::make_shared<mtd::StubSurface>();

    msh::SingleVisibilityFocusMechanism focus_mechanism(mt::fake_shared(model), mt::fake_shared(input_focus_selector));
    
    EXPECT_CALL(*session, default_surface()).Times(1).WillOnce(Return(surface));
    
    EXPECT_CALL(input_focus_selector, set_input_focus_to(Eq(session), Eq(surface))).Times(1);

    model.insert_session(session);
    focus_mechanism.set_focus_to(session);
}

