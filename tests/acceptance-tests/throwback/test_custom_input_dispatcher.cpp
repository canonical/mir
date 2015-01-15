/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "mir/input/input_dispatcher.h"

#include "clients.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/input_testing_server_configuration.h"
#include "mir_test_framework/cross_process_sync.h"
#include "mir_test_doubles/mock_input_dispatcher.h"
#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/event_factory.h"
#include "mir_test/event_matchers.h"

#include "mir/compositor/scene.h"
#include "mir/shell/input_targeter.h"
#include "mir/scene/observer.h"

#include "mir_toolkit/event.h"

#include <linux/input.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mi = mir::input;
namespace mis = mi::synthesis;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mt = mir::test;
namespace mc = mir::compositor;
namespace mtf = mir_test_framework;

namespace
{

class CustomMockInputDispatcher :
    public mtd::MockInputDispatcher,
    public msh::InputTargeter
{
public:
    CustomMockInputDispatcher() = default;
    // mocks for InputTargeter
    MOCK_METHOD1(focus_changed, void(std::shared_ptr<mi::InputChannel const> const& /*focus_channel*/));
    MOCK_METHOD0(focus_cleared, void());
};

struct CustomDispatcherServerConfig : mtf::InputTestingServerConfiguration
{
    std::shared_ptr<mi::InputDispatcher> the_input_dispatcher() override
    {
        auto const dispatcher = the_input_dispatcher_mock();

        if (!expectations_set)
        {
            input_dispatcher_expectations();
            expectations_set = true;
        }

        return dispatcher;
    }

    std::shared_ptr<msh::InputTargeter> the_input_targeter() override
    {
        return the_input_dispatcher_mock();
    }

    std::shared_ptr<::testing::NiceMock<CustomMockInputDispatcher>> the_input_dispatcher_mock()
    {
        return dispatcher(
            [this]
            {
                return std::make_shared<::testing::NiceMock<CustomMockInputDispatcher>>();
            });
    }

    void inject_input() override {}
    virtual void input_dispatcher_expectations() {}

    mir::CachedPtr<::testing::NiceMock<CustomMockInputDispatcher>> dispatcher;
    mtf::CrossProcessSync dispatching_done;
    bool expectations_set{false};
};

}

using CustomInputDispatcher = BespokeDisplayServerTestFixture;

TEST_F(CustomInputDispatcher, receives_input)
{
    struct ServerConfig : CustomDispatcherServerConfig
    {
        void inject_input() override
        {
            fake_event_hub->synthesize_event(mis::a_pointer_event().with_movement(1, 1));
            fake_event_hub->synthesize_event(mis::a_key_down_event().of_scancode(KEY_ENTER));
        }

        void input_dispatcher_expectations() override
        {
            using namespace ::testing;
            auto const dispatcher = the_input_dispatcher_mock();

            InSequence seq;
            EXPECT_CALL(*dispatcher, dispatch(mt::PointerEventWithPosition(1, 1))).Times(1);
            EXPECT_CALL(*dispatcher, dispatch(mt::KeyDownEvent()))
                .WillOnce(InvokeWithoutArgs([this]{ dispatching_done.signal_ready(); }));
        }
    } server_config;

    launch_server_process(server_config);

    // Since event handling happens asynchronously we need to allow some time
    // for it to take place before we leave the test. Otherwise, the server
    // may be stopped before events have been dispatched.
    run_in_test_process([&]
    {
        server_config.dispatching_done.wait_for_signal_ready_for(std::chrono::seconds{50});
    });
}

TEST_F(CustomInputDispatcher, gets_started_and_stopped)
{
    struct ServerConfig : CustomDispatcherServerConfig
    {
        void input_dispatcher_expectations() override
        {
            using namespace ::testing;
            auto const dispatcher = the_input_dispatcher_mock();

            InSequence seq;
            EXPECT_CALL(*dispatcher, start());
            EXPECT_CALL(*dispatcher, stop());
        }
    } server_config;

    launch_server_process(server_config);
}

TEST_F(CustomInputDispatcher, receives_focus_changes)
{
    struct ServerConfig : CustomDispatcherServerConfig
    {
        void input_dispatcher_expectations() override
        {
            using namespace ::testing;
            auto const dispatcher = the_input_dispatcher_mock();

            InSequence seq;
            EXPECT_CALL(*dispatcher, focus_cleared()).Times(1);
            EXPECT_CALL(*dispatcher, focus_changed(_)).Times(1);
            EXPECT_CALL(*dispatcher, focus_cleared()).Times(1)
                .WillOnce(InvokeWithoutArgs([this] { dispatching_done.signal_ready(); }));
        }
    } server_config;

    launch_server_process(server_config);

    mtf::SurfaceCreatingClient client;

    launch_client_process(client);
    run_in_test_process([&]
    {
        server_config.dispatching_done.wait_for_signal_ready_for(std::chrono::seconds{50});
    });
}
