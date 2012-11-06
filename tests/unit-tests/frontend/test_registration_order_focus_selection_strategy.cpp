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
#include "mir/surfaces/surface.h"
#include "mir_test/mock_buffer_bundle.h"
#include "mir_test/mock_application_surface_organiser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;

TEST(RegistrationOrderFocusSelectionStrategy, focus_order)
{
    using namespace ::testing;
    std::shared_ptr<ms::ApplicationSurfaceOrganiser> organiser(new ms::MockApplicationSurfaceOrganiser());
    std::shared_ptr<mf::ApplicationSessionModel> model(new mf::ApplicationSessionModel);
    mf::RegistrationOrderFocusSelectionStrategy focus_selection_strategy(model);
    
    std::shared_ptr<mf::ApplicationSession> app1(new mf::ApplicationSession(organiser, std::string("Visual Studio 7")));
    std::shared_ptr<mf::ApplicationSession> app2(new mf::ApplicationSession(organiser, std::string("Visual Studio 8")));
    std::shared_ptr<mf::ApplicationSession> app3(new mf::ApplicationSession(organiser, std::string("Visual Studio 9")));

    model->insert_session(app1);
    model->insert_session(app2);
    model->insert_session(app3);

    EXPECT_EQ(focus_selection_strategy.next_focus_app(app1).lock()->get_name(), app2->get_name());
    EXPECT_EQ(focus_selection_strategy.next_focus_app(app2).lock()->get_name(), app3->get_name());
    EXPECT_EQ(focus_selection_strategy.next_focus_app(app3).lock()->get_name(), app1->get_name());
}

TEST(RegistrationOrderFocusSelectionStrategy, reverse_focus_order)
{
    using namespace ::testing;
    std::shared_ptr<ms::ApplicationSurfaceOrganiser> organiser(new ms::MockApplicationSurfaceOrganiser());
    std::shared_ptr<mf::ApplicationSessionModel> model(new mf::ApplicationSessionModel);
    mf::RegistrationOrderFocusSelectionStrategy focus_selection_strategy(model);
    
    std::shared_ptr<mf::ApplicationSession> app1(new mf::ApplicationSession(organiser, std::string("Visual Studio 7")));
    std::shared_ptr<mf::ApplicationSession> app2(new mf::ApplicationSession(organiser, std::string("Visual Studio 8")));
    std::shared_ptr<mf::ApplicationSession> app3(new mf::ApplicationSession(organiser, std::string("Visual Studio 9")));

    model->insert_session(app1);
    model->insert_session(app2);
    model->insert_session(app3);

    EXPECT_EQ(focus_selection_strategy.previous_focus_app(app3).lock()->get_name(), app2->get_name());
    EXPECT_EQ(focus_selection_strategy.previous_focus_app(app2).lock()->get_name(), app1->get_name());
    EXPECT_EQ(focus_selection_strategy.previous_focus_app(app1).lock()->get_name(), app3->get_name());
}

TEST(RegistrationOrderFocusSelectionStrategy, no_focus)
{
    using namespace ::testing;
    std::shared_ptr<ms::ApplicationSurfaceOrganiser> organiser(new ms::MockApplicationSurfaceOrganiser());
    std::shared_ptr<mf::ApplicationSessionModel> model(new mf::ApplicationSessionModel);
    mf::RegistrationOrderFocusSelectionStrategy focus_selection_strategy(model);
    
    std::shared_ptr<mf::ApplicationSession> app1(new mf::ApplicationSession(organiser, std::string("Visual Studio 7")));

    model->insert_session(app1);

    EXPECT_EQ(focus_selection_strategy.next_focus_app(std::shared_ptr<mf::ApplicationSession>()).lock()->get_name(), app1->get_name());
}
