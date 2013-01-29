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
#include "mir/sessions/session.h"
#include "mir/sessions/session_container.h"
#include "mir/sessions/registration_order_focus_sequence.h"
#include "mir/sessions/surface_creation_parameters.h"
#include "mir/surfaces/surface.h"
#include "mir_test_doubles/mock_buffer_bundle.h"
#include "mir_test_doubles/mock_surface_factory.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace msess = mir::sessions;
namespace ms = mir::surfaces;
namespace mtd = mir::test::doubles;

TEST(RegistrationOrderFocusSequence, focus_order)
{
    using namespace ::testing;
    auto factory = std::make_shared<mtd::MockSurfaceFactory>();
    auto container = std::make_shared<msess::SessionContainer>();

    auto app1 = std::make_shared<msess::Session>(factory, std::string("Visual Studio 7"));
    auto app2 = std::make_shared<msess::Session>(factory, std::string("Visual Studio 8"));
    auto app3 = std::make_shared<msess::Session>(factory, std::string("Visual Studio 9"));

    container->insert_session(app1);
    container->insert_session(app2);
    container->insert_session(app3);

    msess::RegistrationOrderFocusSequence focus_sequence(container);
    EXPECT_EQ(app2->name(), focus_sequence.successor_of(app1).lock()->name());
    EXPECT_EQ(app3->name(), focus_sequence.successor_of(app2).lock()->name());
    EXPECT_EQ(app1->name(), focus_sequence.successor_of(app3).lock()->name());
}

TEST(RegistrationOrderFocusSequence, reverse_focus_order)
{
    using namespace ::testing;
    auto factory = std::make_shared<mtd::MockSurfaceFactory>();
    auto container = std::make_shared<msess::SessionContainer>();

    auto app1 = std::make_shared<msess::Session>(factory, std::string("Visual Studio 7"));
    auto app2 = std::make_shared<msess::Session>(factory, std::string("Visual Studio 8"));
    auto app3 = std::make_shared<msess::Session>(factory, std::string("Visual Studio 9"));
    container->insert_session(app1);
    container->insert_session(app2);
    container->insert_session(app3);

    msess::RegistrationOrderFocusSequence focus_sequence(container);
    EXPECT_EQ(app2->name(), focus_sequence.predecessor_of(app3).lock()->name());
    EXPECT_EQ(app1->name(), focus_sequence.predecessor_of(app2).lock()->name());
    EXPECT_EQ(app3->name(), focus_sequence.predecessor_of(app1).lock()->name());
}

TEST(RegistrationOrderFocusSequence, no_focus)
{
    using namespace ::testing;

    auto factory = std::make_shared<mtd::MockSurfaceFactory>();
    auto container = std::make_shared<msess::SessionContainer>();

    auto app1 = std::make_shared<msess::Session>(factory, std::string("Visual Studio 7"));
    container->insert_session(app1);

    msess::RegistrationOrderFocusSequence focus_sequence(container);
    EXPECT_EQ(app1->name(), focus_sequence.successor_of(std::shared_ptr<msess::Session>()).lock()->name());
}

TEST(RegistrationOrderFocusSequence, invalid_session_throw_behavior)
{
    using namespace ::testing;

    auto factory = std::make_shared<mtd::MockSurfaceFactory>();
    auto container = std::make_shared<msess::SessionContainer>();
    auto invalid_session = std::make_shared<msess::Session>(factory, std::string("Visual Studio -1"));
    
    msess::RegistrationOrderFocusSequence focus_sequence(container);
    EXPECT_THROW({
            focus_sequence.successor_of(invalid_session);
    }, std::runtime_error);
    EXPECT_THROW({
            focus_sequence.predecessor_of(invalid_session);
    }, std::runtime_error);
}
