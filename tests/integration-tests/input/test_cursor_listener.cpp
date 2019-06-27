/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#include "mir/events/event_private.h"

#include "mir/test/fake_shared.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/fake_input_server_configuration.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/temporary_environment_value.h"
#include "mir/test/doubles/stub_touch_visualizer.h"
#include "mir/test/signal_actions.h"
#include "mir/test/event_factory.h"

#include "mir/input/cursor_listener.h"
#include "mir/input/input_dispatcher.h"
#include "mir/input/input_manager.h"
#include "mir/input/input_device_info.h"
#include "mir_test_framework/executable_path.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>
#include <cstdlib>

namespace mi = mir::input;
namespace mis = mir::input::synthesis;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

namespace
{
using namespace ::testing;

struct MockCursorListener : public mi::CursorListener
{
    MOCK_METHOD2(cursor_moved_to, void(float, float));

    ~MockCursorListener() noexcept {}
};

struct CursorListenerIntegrationTest : testing::Test, mtf::FakeInputServerConfiguration
{
    mtf::TemporaryEnvironmentValue input_lib{"MIR_SERVER_PLATFORM_INPUT_LIB", mtf::server_platform("input-stub.so").c_str()};
    mtf::TemporaryEnvironmentValue real_input{"MIR_SERVER_TESTS_USE_REAL_INPUT", "1"};
    mtf::TemporaryEnvironmentValue no_key_repeat{"MIR_SERVER_ENABLE_KEY_REPEAT", "0"};

    std::shared_ptr<mi::CursorListener> the_cursor_listener() override
    {
        return mt::fake_shared(cursor_listener);
    }

    void SetUp() override
    {
        input_manager = the_input_manager();
        input_manager->start();
        input_dispatcher = the_input_dispatcher();
        input_dispatcher->start();
    }

    void TearDown() override
    {
        input_dispatcher->stop();
        input_manager->stop();
    }

    MockCursorListener cursor_listener;
    std::shared_ptr<mi::InputManager> input_manager;
    std::shared_ptr<mi::InputDispatcher> input_dispatcher;

    std::unique_ptr<mtf::FakeInputDevice> fake_mouse{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"mouse", "mouse-uid" , mi::DeviceCapability::pointer})
        };
};

}

TEST_F(CursorListenerIntegrationTest, cursor_listener_receives_motion)
{
    using namespace ::testing;

    auto signal = std::make_shared<mt::Signal>();

    static const float x = 100.f;
    static const float y = 100.f;

    EXPECT_CALL(cursor_listener, cursor_moved_to(x, y)).Times(1).WillOnce(mt::WakeUp(signal));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(x, y));

    signal->wait_for(std::chrono::seconds{10});
}
