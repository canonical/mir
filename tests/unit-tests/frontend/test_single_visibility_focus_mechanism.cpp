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

#include "mir/compositor/buffer_bundle.h"
#include "mir/frontend/application_session.h"
#include "mir/frontend/application_session_model.h"
#include "mir/frontend/registration_order_focus_selection_strategy.h"
#include "mir/frontend/single_visibility_focus_mechanism.h"
#include "mir/surfaces/surface.h"
#include "mir_test/mock_buffer_bundle.h"
#include "mir_test/empty_deleter.h"
#include "mir_test/mock_application_surface_organiser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;

namespace
{

struct MockApplicationSession : public mf::Session
{
  MockApplicationSession(std::shared_ptr<ms::ApplicationSurfaceOrganiser> organiser,
                         std::string name) : Session(organiser, name)
  {
  }
  MOCK_METHOD0(hide,void());
  MOCK_METHOD0(show,void());
};

}

TEST(SingleVisibilityFocusMechanism, mechanism_sets_visibility)
{
    using namespace ::testing;
    std::shared_ptr<ms::ApplicationSurfaceOrganiser> organiser(new ms::MockApplicationSurfaceOrganiser);
    std::shared_ptr<mf::TheSessionContainerImplementation> model(new mf::TheSessionContainerImplementation);
    
    MockApplicationSession m1(organiser, "Visual Studio 7");
    MockApplicationSession m2(organiser, "Visual Studio 8");
    MockApplicationSession m3(organiser, "Visual Studio 9");
    
    std::shared_ptr<mf::Session> app1(&m1, mir::EmptyDeleter());
    std::shared_ptr<mf::Session> app2(&m2, mir::EmptyDeleter());
    std::shared_ptr<mf::Session> app3(&m3, mir::EmptyDeleter());

    mf::SingleVisibilityFocusMechanism focus_mechanism(model);
    
    EXPECT_CALL(m1, show()).Times(1);
    EXPECT_CALL(m2, hide()).Times(1);
    EXPECT_CALL(m3, hide()).Times(1);

    model->insert_session(app1);
    model->insert_session(app2);
    model->insert_session(app3);

    focus_mechanism.set_focus_to(app1);
}

