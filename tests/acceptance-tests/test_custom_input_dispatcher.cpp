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
#include "mir/input/input_dispatcher_configuration.h"

#include "clients.h"

#include "mir/shell/input_targeter.h"
#include "mir/scene/input_registrar.h"

#include "mir_toolkit/event.h"

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/input_testing_server_configuration.h"
#include "mir_test_doubles/mock_input_dispatcher.h"
#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h"
#include "mir_test/event_factory.h"
#include "mir_test/client_event_matchers.h"

#include <linux/input.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace msh = mir::shell;
namespace ms = mir::scene;
namespace mi = mir::input;
namespace mis = mi::synthesis;
namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

class CustomInputDispatcher :
    public mtd::MockInputDispatcher,
    public ms::InputRegistrar,
    public msh::InputTargeter
{
public:
    MOCK_METHOD3(input_channel_opened, void(std::shared_ptr<mi::InputChannel> const& /*opened_channel*/,
                              std::shared_ptr<mi::Surface> const& /*info*/,
                              mi::InputReceptionMode /*input_mode*/));
    MOCK_METHOD1(input_channel_closed, void(std::shared_ptr<mi::InputChannel> const& /*closed_channel*/));
    // empty stubs for InputRegistrar and InputTargeter
    MOCK_METHOD1(focus_changed, void(std::shared_ptr<mi::InputChannel const> const& /*focus_channel*/));
    MOCK_METHOD0(focus_cleared, void());
};

class CustomInputDispatcherConfiguration : public mi::InputDispatcherConfiguration
{
public:
    std::shared_ptr<ms::InputRegistrar> the_input_registrar() override
    {
        return mt::fake_shared<ms::InputRegistrar>(dispatcher);
    }
    std::shared_ptr<msh::InputTargeter> the_input_targeter() override
    {
        return mt::fake_shared<msh::InputTargeter>(dispatcher);
    }
    std::shared_ptr<mi::InputDispatcher> the_input_dispatcher()  override
    {
        return mt::fake_shared<mi::InputDispatcher>(dispatcher);
    }

    bool is_key_repeat_enabled() const override { return false; }

    void set_input_targets(std::shared_ptr<mi::InputTargets> const& /*targets*/) override
    {}

    ::testing::NiceMock<CustomInputDispatcher> dispatcher;
};


TEST_F(BespokeDisplayServerTestFixture, custom_input_dispatcher_receives_input)
{
    struct ServerConfig : mtf::InputTestingServerConfiguration
    {
        std::shared_ptr<mi::InputDispatcherConfiguration>
        the_input_dispatcher_configuration() override
        {
            return input_dispatcher_configuration(
            []
            {
                auto dispatcher_conf = std::make_shared<CustomInputDispatcherConfiguration>();
                {
                    using namespace ::testing;
                    InSequence seq;
                    EXPECT_CALL(dispatcher_conf->dispatcher, dispatch(mt::MotionEventWithPosition(1, 1))).Times(1);
                    EXPECT_CALL(dispatcher_conf->dispatcher, dispatch(mt::KeyDownEvent())).Times(1);
                }

                return dispatcher_conf;
            });
        }
        void inject_input() override
        {
            fake_event_hub->synthesize_event(mis::a_motion_event().with_movement(1, 1));
            fake_event_hub->synthesize_event(mis::a_key_down_event().of_scancode(KEY_ENTER));
        }
    } server_config;

    launch_server_process(server_config);
}

TEST_F(BespokeDisplayServerTestFixture, custom_input_dispatcher_receives_opened_sessions)
{
    struct ServerConfig : mtf::InputTestingServerConfiguration
    {
        std::shared_ptr<mi::InputDispatcherConfiguration>
        the_input_dispatcher_configuration() override
        {
            return input_dispatcher_configuration(
            []
            {
                auto dispatcher_conf = std::make_shared<CustomInputDispatcherConfiguration>();
                {
                    using namespace ::testing;
                    InSequence seq;

                    EXPECT_CALL(dispatcher_conf->dispatcher, input_channel_opened(_,_,_)).Times(1);
                    EXPECT_CALL(dispatcher_conf->dispatcher, input_channel_closed(_)).Times(1);
                }

                return dispatcher_conf;
            });
        }
        void inject_input() override {}
    } server_config;

    launch_server_process(server_config);

    mtf::SurfaceCreatingClient client;

    launch_client_process(client);
}


TEST_F(BespokeDisplayServerTestFixture, custom_input_dispatcher_receives_focus_changes)
{
    struct ServerConfig : mtf::InputTestingServerConfiguration
    {
        std::shared_ptr<mi::InputDispatcherConfiguration>
        the_input_dispatcher_configuration() override
        {
            return input_dispatcher_configuration(
            []
            {
                auto dispatcher_conf = std::make_shared<CustomInputDispatcherConfiguration>();
                {
                    using namespace ::testing;
                    InSequence seq;

                    EXPECT_CALL(dispatcher_conf->dispatcher, focus_cleared()).Times(1);
                    EXPECT_CALL(dispatcher_conf->dispatcher, focus_changed(_)).Times(1);
                    EXPECT_CALL(dispatcher_conf->dispatcher, focus_cleared()).Times(1);
                }

                return dispatcher_conf;
            });
        }
        void inject_input() override {}
    } server_config;

    launch_server_process(server_config);

    mtf::SurfaceCreatingClient client;

    launch_client_process(client);
}
