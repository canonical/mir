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
#include "mir/frontend/the_session_container_implementation.h"
#include "mir/frontend/registration_order_focus_sequence.h"
#include "mir/surfaces/surface.h"
#include "mir_test/mock_buffer_bundle.h"
#include "mir_test/mock_application_surface_organiser.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace ms = mir::surfaces;

TEST(RegistrationOrderFocusSequence, focus_order)
{
    using namespace ::testing;
    std::shared_ptr<mf::SurfaceOrganiser> organiser(new mf::MockSurfaceOrganiser());
    std::shared_ptr<mf::SessionContainer> model(new mf::TheSessionContainerImplementation);
    mf::RegistrationOrderFocusSequence focus_sequence(model);
    
    std::shared_ptr<mf::Session> app1(new mf::Session(organiser, std::string("Visual Studio 7")));
    std::shared_ptr<mf::Session> app2(new mf::Session(organiser, std::string("Visual Studio 8")));
    std::shared_ptr<mf::Session> app3(new mf::Session(organiser, std::string("Visual Studio 9")));

    model->insert_session(app1);
    model->insert_session(app2);
    model->insert_session(app3);

    EXPECT_EQ(focus_sequence.successor_of(app1).lock()->get_name(), app2->get_name());
    EXPECT_EQ(focus_sequence.successor_of(app2).lock()->get_name(), app3->get_name());
    EXPECT_EQ(focus_sequence.successor_of(app3).lock()->get_name(), app1->get_name());
}

TEST(RegistrationOrderFocusSequence, reverse_focus_order)
{
    using namespace ::testing;
    std::shared_ptr<mf::SurfaceOrganiser> organiser(new mf::MockSurfaceOrganiser());
    std::shared_ptr<mf::SessionContainer> container(new mf::TheSessionContainerImplementation);
    mf::RegistrationOrderFocusSequence focus_sequence(container);
    
    std::shared_ptr<mf::Session> app1(new mf::Session(organiser, std::string("Visual Studio 7")));
    std::shared_ptr<mf::Session> app2(new mf::Session(organiser, std::string("Visual Studio 8")));
    std::shared_ptr<mf::Session> app3(new mf::Session(organiser, std::string("Visual Studio 9")));

    container->insert_session(app1);
    container->insert_session(app2);
    container->insert_session(app3);

    EXPECT_EQ(focus_sequence.predecessor_of(app3).lock()->get_name(), app2->get_name());
    EXPECT_EQ(focus_sequence.predecessor_of(app2).lock()->get_name(), app1->get_name());
    EXPECT_EQ(focus_sequence.predecessor_of(app1).lock()->get_name(), app3->get_name());
}

TEST(RegistrationOrderFocusSequence, no_focus)
{
    using namespace ::testing;
    std::shared_ptr<mf::SurfaceOrganiser> organiser(new mf::MockSurfaceOrganiser());
    std::shared_ptr<mf::SessionContainer> container(new mf::TheSessionContainerImplementation);
    mf::RegistrationOrderFocusSequence focus_sequence(container);
    
    std::shared_ptr<mf::Session> app1(new mf::Session(organiser, std::string("Visual Studio 7")));

    container->insert_session(app1);

    EXPECT_EQ(focus_sequence.successor_of(std::shared_ptr<mf::Session>()).lock()->get_name(), app1->get_name());
}
