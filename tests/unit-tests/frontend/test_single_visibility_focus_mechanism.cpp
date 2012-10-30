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
#include "mir/surfaces/application_surface_organiser.h"
#include "mir/frontend/registration_order_focus_strategy.h"
#include "mir/frontend/single_visibility_focus_mechanism.h"
#include "mir/surfaces/surface.h"
#include "mir_test/mock_buffer_bundle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;

namespace
{

struct MockApplicationSurfaceOrganiser : public ms::ApplicationSurfaceOrganiser
{
    MOCK_METHOD1(create_surface, std::weak_ptr<ms::Surface>(const ms::SurfaceCreationParameters&));
    MOCK_METHOD1(destroy_surface, void(std::weak_ptr<ms::Surface> surface));
    MOCK_METHOD2(hide_surface, void(std::weak_ptr<ms::Surface>,bool));
};

}

TEST(RegistrationOrderFocusStrategy, mechanism_hides_unfocused_apps)
{
    using namespace ::testing;
    MockApplicationSurfaceOrganiser organiser;
    std::shared_ptr<mf::ApplicationSessionModel> model(new mf::ApplicationSessionModel);
    
    std::shared_ptr<mf::ApplicationSession> app1(new mf::ApplicationSession(&organiser, std::string("Visual Studio 7")));
    std::shared_ptr<mf::ApplicationSession> app2(new mf::ApplicationSession(&organiser, std::string("Visual Studio 8")));
    std::shared_ptr<mf::ApplicationSession> app3(new mf::ApplicationSession(&organiser, std::string("Visual Studio 9")));

    model->insert_session(app1);
    model->insert_session(app2);
    model->insert_session(app3);

    mf::SingleVisibilityFocusMechanism focus_mechanism;
    
    EXPECT_CALL(organiser, hide_surface(_, false)).Times(1);
    EXPECT_CALL(organiser, hide_surface(_, true)).Times(1);
    EXPECT_CALL(organiser, hide_surface(_, true)).Times(1);
    
    focus_mechanism.focus(model, app1);
}

