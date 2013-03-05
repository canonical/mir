/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/input/android/android_input_manager.h"
#include "src/input/android/android_input_configuration.h"
#include "src/input/android/android_input_thread.h"
#include "src/input/android/android_input_constants.h"

#include "mir/input/cursor_listener.h"

#include "mir_test_doubles/mock_viewable_area.h"
#include "mir_test/fake_shared.h"
#include "mir_test/fake_event_hub.h" // TODO: Replace with mocked event hub ~ racarr

#include <InputDispatcher.h>
#include <InputListener.h>
#include <InputReader.h>
#include <EventHub.h>
#include <utils/StrongPointer.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <initializer_list>

namespace droidinput = android;

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mt = mir::test;
namespace mtd = mt::doubles;

// Test constants
namespace
{
static const std::shared_ptr<mi::CursorListener> null_cursor_listener{};
static std::initializer_list<std::shared_ptr<mi::EventFilter> const> empty_event_filters{};
static const geom::Rectangle default_view_area =
    geom::Rectangle{geom::Point(),
                    geom::Size{geom::Width(1600), geom::Height(1400)}};
}


// Mock objects
namespace
{

struct MockInputConfiguration : public mia::InputConfiguration
{
    MOCK_METHOD0(the_event_hub, droidinput::sp<droidinput::EventHubInterface>());
    // TODO: Might not belong ~racarr
    MOCK_METHOD0(the_dispatcher_policy, droidinput::sp<droidinput::InputDispatcherPolicyInterface>());
    MOCK_METHOD0(the_dispatcher, droidinput::sp<droidinput::InputDispatcherInterface>());
    MOCK_METHOD0(the_reader, droidinput::sp<droidinput::InputReaderInterface>());
    MOCK_METHOD0(the_dispatcher_thread, std::shared_ptr<mia::InputThread>());
    MOCK_METHOD0(the_reader_thread, std::shared_ptr<mia::InputThread>());
};

struct MockInputDispatcher : public droidinput::InputDispatcherInterface
{
    // droidinput::InputDispatcher interface
    MOCK_METHOD1(dump, void(droidinput::String8&));
    MOCK_METHOD0(monitor, void());
    MOCK_METHOD0(dispatchOnce, void());
    MOCK_METHOD6(injectInputEvent, int32_t(droidinput::InputEvent const*, int32_t, int32_t, int32_t, int32_t, uint32_t));
    MOCK_METHOD1(setInputWindows, void(droidinput::Vector<droidinput::sp<droidinput::InputWindowHandle>> const&));
    MOCK_METHOD1(setFocusedApplication, void(droidinput::sp<droidinput::InputApplicationHandle> const&));
    MOCK_METHOD2(setInputDispatchMode, void(bool, bool));
    MOCK_METHOD1(setInputFilterEnabled, void(bool));
    MOCK_METHOD2(transferTouchFocus, bool(droidinput::sp<droidinput::InputChannel> const&, droidinput::sp<droidinput::InputChannel> const&));
    MOCK_METHOD3(registerInputChannel, droidinput::status_t(droidinput::sp<droidinput::InputChannel> const&, droidinput::sp<droidinput::InputWindowHandle> const&, bool));
    MOCK_METHOD1(unregisterInputChannel, droidinput::status_t(droidinput::sp<droidinput::InputChannel> const&));

    // droidinput::InputListener interface
    MOCK_METHOD1(notifyConfigurationChanged, void(droidinput::NotifyConfigurationChangedArgs const*));
    MOCK_METHOD1(notifyKey, void(droidinput::NotifyKeyArgs const*));
    MOCK_METHOD1(notifyMotion, void(droidinput::NotifyMotionArgs const*));
    MOCK_METHOD1(notifySwitch, void(droidinput::NotifySwitchArgs const*));
    MOCK_METHOD1(notifyDeviceReset, void(droidinput::NotifyDeviceResetArgs const*));
};

struct MockInputReader : public droidinput::InputReaderInterface
{
    MOCK_METHOD1(dump, void(droidinput::String8&));
    MOCK_METHOD0(monitor, void());
    MOCK_METHOD0(loopOnce, void());
    MOCK_METHOD1(getInputDevices, void(droidinput::Vector<droidinput::InputDeviceInfo>&));
    MOCK_METHOD3(getScanCodeState, int32_t(int32_t, uint32_t, int32_t));
    MOCK_METHOD3(getKeyCodeState, int32_t(int32_t, uint32_t, int32_t));
    MOCK_METHOD3(getSwitchState, int32_t(int32_t, uint32_t, int32_t));
    MOCK_METHOD5(hasKeys, bool(int32_t, uint32_t, size_t, int32_t const*, uint8_t*));
    MOCK_METHOD1(requestRefreshConfiguration, void(uint32_t));
    MOCK_METHOD5(vibrate, void(int32_t, nsecs_t const*, size_t, ssize_t, int32_t));
    MOCK_METHOD2(cancelVibrate, void(int32_t, int32_t));
};

struct MockInputThread : public mia::InputThread
{
    MOCK_METHOD0(start, void());
    MOCK_METHOD0(request_stop, void());
    MOCK_METHOD0(join, void());
};

}

// Test fixture
namespace
{

struct AndroidInputManagerSetup : public testing::Test
{
    void SetUp()
    {
        using namespace ::testing;

        ON_CALL(view_area, view_area())
            .WillByDefault(Return(default_view_area));

        event_hub = new mia::FakeEventHub(); // TODO: Replace with mock ~racarr
        dispatcher = new MockInputDispatcher();
        reader = new MockInputReader();
        dispatcher_thread = std::make_shared<MockInputThread>();
        reader_thread = std::make_shared<MockInputThread>();

        ON_CALL(config, the_event_hub()).WillByDefault(Return(event_hub));
        ON_CALL(config, the_dispatcher()).WillByDefault(Return(dispatcher));
        ON_CALL(config, the_reader()).WillByDefault(Return(reader));
        ON_CALL(config, the_reader_thread()).WillByDefault(Return(reader_thread));
        ON_CALL(config, the_dispatcher_thread()).WillByDefault(Return(dispatcher_thread));
    }
    mtd::MockViewableArea view_area;

    testing::NiceMock<MockInputConfiguration> config;
    droidinput::sp<droidinput::EventHubInterface> event_hub;
    droidinput::sp<MockInputDispatcher> dispatcher;
    droidinput::sp<MockInputReader> reader;
    std::shared_ptr<MockInputThread> dispatcher_thread;
    std::shared_ptr<MockInputThread> reader_thread;
};

}

TEST_F(AndroidInputManagerSetup, takes_input_setup_from_configuration)
{
    // TODO: Split to two tests and fixture with default actions and nicemock for config.
    using namespace ::testing;
    
    EXPECT_CALL(config, the_event_hub()).Times(1);
    EXPECT_CALL(config, the_dispatcher()).Times(1);
    EXPECT_CALL(config, the_reader()).Times(1);
    EXPECT_CALL(config, the_reader_thread()).Times(1);
    EXPECT_CALL(config, the_dispatcher_thread()).Times(1);
    
    mia::InputManager manager(mt::fake_shared(config));
    
}

TEST_F(AndroidInputManagerSetup, start_and_stop)
{
    using namespace ::testing;
    
    EXPECT_CALL(*dispatcher, setInputDispatchMode(mia::DispatchEnabled, mia::DispatchUnfrozen)).Times(1);
    EXPECT_CALL(*dispatcher, setInputFilterEnabled(true)).Times(1);

    EXPECT_CALL(*reader_thread, start()).Times(1);
    EXPECT_CALL(*dispatcher_thread, start()).Times(1);
    
    {
        InSequence seq;
        
        EXPECT_CALL(*dispatcher_thread, request_stop());
        EXPECT_CALL(*dispatcher, setInputDispatchMode(mia::DispatchDisabled, mia::DispatchFrozen)).Times(1);
        EXPECT_CALL(*dispatcher_thread, join());
        EXPECT_CALL(*reader_thread, request_stop());
        // TODO: Expectation on event hub wake
        EXPECT_CALL(*reader_thread, join());
    }

    mia::InputManager manager(mt::fake_shared(config));

    manager.start();
    manager.stop();
}
