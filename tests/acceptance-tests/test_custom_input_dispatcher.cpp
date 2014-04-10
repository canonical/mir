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

namespace
{
    char const* const mir_test_socket = mtf::test_socket_file().c_str();
}

class CustomInputDispatcher :
    public mtd::MockInputDispatcher,
    public ms::InputRegistrar,
    public msh::InputTargeter
{
public:
    // empty stubs for InputRegistrar and InputTargeter
    void input_channel_opened(std::shared_ptr<mi::InputChannel> const& /*opened_channel*/,
                              std::shared_ptr<mi::Surface> const& /*info*/,
                              mi::InputReceptionMode /*input_mode*/) override
    {
    }
    void input_channel_closed(std::shared_ptr<mi::InputChannel> const& /*closed_channel*/) override
    {
    }
    void focus_changed(std::shared_ptr<mi::InputChannel const> const& /*focus_channel*/) override
    {
    }
    void focus_cleared() override
    {
    }
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

    CustomInputDispatcher dispatcher;
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

