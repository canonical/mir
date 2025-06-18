/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/fake_input_server_configuration.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/temporary_environment_value.h"
#include "mir/test/auto_unblock_thread.h"
#include "mir/test/signal_actions.h"
#include "mir/test/event_factory.h"

#include "mir/main_loop.h"
#include "mir/input/cursor_observer.h"
#include "mir/input/cursor_observer_multiplexer.h"
#include "mir/input/input_dispatcher.h"
#include "mir/input/input_manager.h"
#include "mir/input/input_device_info.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mi = mir::input;
namespace mis = mir::input::synthesis;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

namespace
{
using namespace ::testing;

struct MockCursorObserver : public mi::CursorObserver
{
    MOCK_METHOD2(cursor_moved_to, void(float, float));

    ~MockCursorObserver() noexcept {}

    void pointer_usable() {}
    void pointer_unusable() {}
};

struct CursorObserverIntegrationTest : testing::Test, mtf::FakeInputServerConfiguration
{
    mtf::TemporaryEnvironmentValue input_lib{"MIR_SERVER_PLATFORM_INPUT_LIB", "mir:stub-input"};
    mtf::TemporaryEnvironmentValue real_input{"MIR_SERVER_TESTS_USE_REAL_INPUT", "1"};
    mtf::TemporaryEnvironmentValue no_key_repeat{"MIR_SERVER_ENABLE_KEY_REPEAT", "0"};

    void SetUp() override
    {
        input_manager->start();
        input_dispatcher->start();

        cursor_observer_multiplexer->register_interest(cursor_observer);
    }

    void TearDown() override
    {
        input_dispatcher->stop();
        input_manager->stop();
    }

    std::shared_ptr<MockCursorObserver> cursor_observer{std::make_shared<MockCursorObserver>()};
    std::shared_ptr<mi::InputManager> input_manager{the_input_manager()};
    std::shared_ptr<mi::InputDispatcher> const input_dispatcher{the_input_dispatcher()};
    std::shared_ptr<mir::MainLoop> const main_loop{the_main_loop()};
    std::shared_ptr<mi::CursorObserverMultiplexer> cursor_observer_multiplexer{the_cursor_observer_multiplexer()};

    std::unique_ptr<mtf::FakeInputDevice> fake_mouse{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"mouse", "mouse-uid" , mi::DeviceCapability::pointer})
        };

    mir::test::AutoUnblockThread main_thread{
        [this] { main_loop->stop(); },
        [this]
        {
            main_loop->run();
        }};
};

}

TEST_F(CursorObserverIntegrationTest, cursor_observer_receives_motion)
{
    using namespace ::testing;

    auto signal = std::make_shared<mt::Signal>();

    static const float x = 100.f;
    static const float y = 100.f;

    EXPECT_CALL(*cursor_observer, cursor_moved_to(x, y)).Times(1).WillOnce(mt::WakeUp(signal));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(x, y));

    signal->wait_for(std::chrono::seconds{10});
}
