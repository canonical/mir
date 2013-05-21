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
#include "mir/shell/default_session_container.h"
#include "mir/shell/registration_order_focus_sequence.h"
#include "mir/shell/surface_creation_parameters.h"
#include "mir/surfaces/surface.h"

#include "mir_test_doubles/mock_buffer_bundle.h"
#include "mir_test_doubles/mock_surface_factory.h"
#include "mir_test_doubles/stub_input_target_listener.h"
#include "mir_test/fake_shared.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>

namespace mc = mir::compositor;
namespace me = mir::events;
namespace msh = mir::shell;
namespace ms = mir::surfaces;
namespace mt = mir::test;
namespace mtd = mt::doubles;

namespace
{
struct RegistrationOrderFocusSequenceSetup : public testing::Test
{
    void SetUp()
    {
        factory = std::make_shared<mtd::MockSurfaceFactory>();
        container = std::make_shared<msh::DefaultSessionContainer>();
    }
    std::shared_ptr<mtd::MockSurfaceFactory> factory;
    std::shared_ptr<msh::DefaultSessionContainer> container;
    mtd::StubInputTargetListener input_listener;

    static std::string const testing_app_name1;
    static std::string const testing_app_name2;
    static std::string const testing_app_name3;
};

std::string const RegistrationOrderFocusSequenceSetup::testing_app_name1{"Visual Studio 7"};
std::string const RegistrationOrderFocusSequenceSetup::testing_app_name2{"Visual Studio 8"};
std::string const RegistrationOrderFocusSequenceSetup::testing_app_name3{"Visual Studio 9"};
}

TEST_F(RegistrationOrderFocusSequenceSetup, focus_order)
{
    using namespace ::testing;

    auto app1 = std::make_shared<msh::ApplicationSession>(factory, mt::fake_shared(input_listener), testing_app_name1, std::shared_ptr<me::EventSink>());
    auto app2 = std::make_shared<msh::ApplicationSession>(factory, mt::fake_shared(input_listener), testing_app_name2, std::shared_ptr<me::EventSink>());
    auto app3 = std::make_shared<msh::ApplicationSession>(factory, mt::fake_shared(input_listener), testing_app_name3, std::shared_ptr<me::EventSink>());

    container->insert_session(app1);
    container->insert_session(app2);
    container->insert_session(app3);

    msh::RegistrationOrderFocusSequence focus_sequence(container);
    EXPECT_EQ(app2, focus_sequence.successor_of(app1));
    EXPECT_EQ(app3, focus_sequence.successor_of(app2));
    EXPECT_EQ(app1, focus_sequence.successor_of(app3));
}

TEST_F(RegistrationOrderFocusSequenceSetup, reverse_focus_order)
{
    using namespace ::testing;

    auto app1 = std::make_shared<msh::ApplicationSession>(factory, mt::fake_shared(input_listener), testing_app_name1, std::shared_ptr<me::EventSink>());
    auto app2 = std::make_shared<msh::ApplicationSession>(factory, mt::fake_shared(input_listener), testing_app_name2, std::shared_ptr<me::EventSink>());
    auto app3 = std::make_shared<msh::ApplicationSession>(factory, mt::fake_shared(input_listener), testing_app_name3, std::shared_ptr<me::EventSink>());
    container->insert_session(app1);
    container->insert_session(app2);
    container->insert_session(app3);

    msh::RegistrationOrderFocusSequence focus_sequence(container);
    EXPECT_EQ(app2, focus_sequence.predecessor_of(app3));
    EXPECT_EQ(app1, focus_sequence.predecessor_of(app2));
    EXPECT_EQ(app3, focus_sequence.predecessor_of(app1));
}

TEST_F(RegistrationOrderFocusSequenceSetup, identity)
{
    using namespace ::testing;

    auto app1 = std::make_shared<msh::ApplicationSession>(factory, mt::fake_shared(input_listener), testing_app_name1, std::shared_ptr<me::EventSink>());
    container->insert_session(app1);

    msh::RegistrationOrderFocusSequence focus_sequence(container);
    EXPECT_EQ(app1, focus_sequence.predecessor_of(app1));
    EXPECT_EQ(app1, focus_sequence.successor_of(app1));
}

TEST_F(RegistrationOrderFocusSequenceSetup, default_focus)
{
    using namespace ::testing;

    auto app1 = std::make_shared<msh::ApplicationSession>(factory, mt::fake_shared(input_listener), testing_app_name1, std::shared_ptr<me::EventSink>());
    auto app2 = std::make_shared<msh::ApplicationSession>(factory, mt::fake_shared(input_listener), testing_app_name2, std::shared_ptr<me::EventSink>());
    auto null_session = std::shared_ptr<msh::ApplicationSession>();

    msh::RegistrationOrderFocusSequence focus_sequence(container);

    EXPECT_EQ(null_session, focus_sequence.default_focus());
    container->insert_session(app1);
    EXPECT_EQ(app1, focus_sequence.default_focus());
    container->insert_session(app2);
    EXPECT_EQ(app2, focus_sequence.default_focus());
}

TEST_F(RegistrationOrderFocusSequenceSetup, invalid_session_throw_behavior)
{
    using namespace ::testing;

    auto invalid_session = std::make_shared<msh::ApplicationSession>(factory, mt::fake_shared(input_listener), testing_app_name1, std::shared_ptr<me::EventSink>());
    auto null_session = std::shared_ptr<msh::ApplicationSession>();

    msh::RegistrationOrderFocusSequence focus_sequence(container);

    EXPECT_THROW({
            focus_sequence.successor_of(null_session);
    }, std::logic_error);
    EXPECT_THROW({
            focus_sequence.predecessor_of(null_session);
    }, std::logic_error);
    EXPECT_THROW({
            focus_sequence.successor_of(invalid_session);
    }, std::logic_error);
    EXPECT_THROW({
            focus_sequence.predecessor_of(invalid_session);
    }, std::logic_error);
}
