/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "src/server/input/android/android_input_manager.h"
#include "src/server/input/android/android_input_thread.h"
#include "src/server/input/android/android_input_constants.h"
#include "src/server/input/android/android_input_channel.h"

#include "mir/input/input_channel.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_input_dispatcher.h"
#include "mir_test_doubles/stub_input_channel.h"

#include <EventHub.h>
#include <utils/StrongPointer.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <initializer_list>

namespace droidinput = android;

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mt = mir::test;
namespace mtd = mt::doubles;

// Mock objects
namespace
{

struct MockEventHub : public droidinput::EventHubInterface
{
    MOCK_CONST_METHOD1(getDeviceClasses, uint32_t(int32_t));
    MOCK_CONST_METHOD1(getDeviceIdentifier, droidinput::InputDeviceIdentifier(int32_t));
    MOCK_CONST_METHOD2(getConfiguration, void(int32_t, droidinput::PropertyMap*));
    MOCK_CONST_METHOD3(getAbsoluteAxisInfo, droidinput::status_t(int32_t, int, droidinput::RawAbsoluteAxisInfo*));
    MOCK_CONST_METHOD2(hasRelativeAxis, bool(int32_t, int));
    MOCK_CONST_METHOD2(hasInputProperty, bool(int32_t, int));
    MOCK_CONST_METHOD5(mapKey, droidinput::status_t(int32_t, int32_t, int32_t, int32_t*, uint32_t*));
    MOCK_CONST_METHOD3(mapAxis, droidinput::status_t(int32_t, int32_t, droidinput::AxisInfo*));
    MOCK_METHOD1(setExcludedDevices, void(droidinput::Vector<droidinput::String8> const&));
    MOCK_METHOD3(getEvents, size_t(int, droidinput::RawEvent*, size_t));
    MOCK_CONST_METHOD2(getScanCodeState, int32_t(int32_t, int32_t));
    MOCK_CONST_METHOD2(getKeyCodeState, int32_t(int32_t, int32_t));
    MOCK_CONST_METHOD2(getSwitchState, int32_t(int32_t, int32_t));
    MOCK_CONST_METHOD3(getAbsoluteAxisValue, droidinput::status_t(int32_t, int32_t, int32_t*));
    MOCK_CONST_METHOD4(markSupportedKeyCodes, bool(int32_t, size_t, int32_t const*, uint8_t*));
    MOCK_CONST_METHOD2(hasScanCode, bool(int32_t, int32_t));
    MOCK_CONST_METHOD2(hasLed, bool(int32_t, int32_t));
    MOCK_METHOD3(setLedState, void(int32_t, int32_t, bool));
    MOCK_CONST_METHOD2(getVirtualKeyDefinitions, void(int32_t, droidinput::Vector<droidinput::VirtualKeyDefinition>&));
    MOCK_CONST_METHOD1(getKeyCharacterMap, droidinput::sp<droidinput::KeyCharacterMap>(int32_t));
    MOCK_METHOD2(setKeyboardLayoutOverlay, bool(int32_t, const droidinput::sp<droidinput::KeyCharacterMap>&));
    MOCK_METHOD2(vibrate, void(int32_t, nsecs_t));
    MOCK_METHOD1(cancelVibrate, void(int32_t));
    MOCK_METHOD0(requestReopenDevices, void());
    MOCK_METHOD0(wake, void());
    MOCK_METHOD1(dump, void(droidinput::String8&));
    MOCK_METHOD0(monitor, void());
    MOCK_METHOD0(flush, void());
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
    AndroidInputManagerSetup()
    {
        using namespace ::testing;

        event_hub = new MockEventHub();
        dispatcher = new mtd::MockInputDispatcher();
        dispatcher_thread = std::make_shared<MockInputThread>();
        reader_thread = std::make_shared<MockInputThread>();
    }

    droidinput::sp<MockEventHub> event_hub;
    droidinput::sp<mtd::MockInputDispatcher> dispatcher;
    std::shared_ptr<MockInputThread> dispatcher_thread;
    std::shared_ptr<MockInputThread> reader_thread;
};

}

TEST_F(AndroidInputManagerSetup, start_and_stop)
{
    using namespace ::testing;

    ExpectationSet dispatcher_setup;
    ExpectationSet reader_setup;

    dispatcher_setup += EXPECT_CALL(*dispatcher,
                                    setInputDispatchMode(mia::DispatchEnabled,
                                                         mia::DispatchUnfrozen))
                            .Times(1);
    dispatcher_setup += EXPECT_CALL(*dispatcher, setInputFilterEnabled(true)).Times(1);

    reader_setup += EXPECT_CALL(*event_hub, flush()).Times(1);

    EXPECT_CALL(*reader_thread, start())
        .Times(1)
        .After(reader_setup);

    EXPECT_CALL(*dispatcher_thread, start())
        .Times(1)
        .After(dispatcher_setup);

    {
        InSequence seq;

        EXPECT_CALL(*dispatcher_thread, request_stop());
        EXPECT_CALL(*dispatcher, setInputDispatchMode(mia::DispatchDisabled, mia::DispatchFrozen)).Times(1);
        EXPECT_CALL(*dispatcher_thread, join());
        EXPECT_CALL(*reader_thread, request_stop());
        EXPECT_CALL(*event_hub, wake());
        EXPECT_CALL(*reader_thread, join());
    }

    mia::InputManager manager(event_hub, dispatcher, reader_thread, dispatcher_thread);

    manager.start();
    manager.stop();
}

TEST_F(AndroidInputManagerSetup, manager_returns_input_channel_with_fds)
{
    mia::InputManager manager(event_hub, dispatcher, reader_thread, dispatcher_thread);

    auto package = manager.make_input_channel();
    EXPECT_NE(nullptr, std::dynamic_pointer_cast<mia::AndroidInputChannel>(package));
}
