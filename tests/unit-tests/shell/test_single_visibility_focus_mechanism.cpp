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
#include "mir/shell/surface_creation_parameters.h"
#include "mir/surfaces/surface.h"
#include "mir_test_doubles/mock_buffer_bundle.h"
#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/mock_session.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

TEST(SingleVisibilityFocusMechanism, mechanism_sets_visibility)
{
    using namespace ::testing;

    mtd::MockSession app1;
    mtd::MockSession app2;
    mtd::MockSession app3;

    msh::SessionContainer model;
    msh::SingleVisibilityFocusMechanism focus_mechanism(mt::fake_shared(model));

    EXPECT_CALL(app1, show()).Times(1);
    EXPECT_CALL(app2, hide()).Times(1);
    EXPECT_CALL(app3, hide()).Times(1);

    model.insert_session(mt::fake_shared(app1));
    model.insert_session(mt::fake_shared(app2));
    model.insert_session(mt::fake_shared(app3));

    focus_mechanism.set_focus_to(mt::fake_shared(app1));
}

