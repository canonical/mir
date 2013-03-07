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

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;

namespace
{

struct MockApplicationSession : public msh::ApplicationSession
{
  MockApplicationSession(std::shared_ptr<msh::SurfaceFactory> factory,
                         std::string name) : ApplicationSession(factory, name)
  {
  }
  MOCK_METHOD0(hide,void());
  MOCK_METHOD0(show,void());
};

}

TEST(SingleVisibilityFocusMechanism, mechanism_sets_visibility)
{
    using namespace ::testing;
    std::shared_ptr<msh::SurfaceFactory> factory(new mtd::MockSurfaceFactory);
    std::shared_ptr<msh::SessionContainer> model(new msh::SessionContainer);

    MockApplicationSession m1(factory, "Visual Studio 7");
    MockApplicationSession m2(factory, "Visual Studio 8");
    MockApplicationSession m3(factory, "Visual Studio 9");

    auto app1 = mt::fake_shared(m1);
    auto app2 = mt::fake_shared(m2);
    auto app3 = mt::fake_shared(m3);

    msh::SingleVisibilityFocusMechanism focus_mechanism(model);

    EXPECT_CALL(m1, show()).Times(1);
    EXPECT_CALL(m2, hide()).Times(1);
    EXPECT_CALL(m3, hide()).Times(1);

    model->insert_session(app1);
    model->insert_session(app2);
    model->insert_session(app3);

    focus_mechanism.set_focus_to(app1);
}

