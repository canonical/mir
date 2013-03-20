/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "src/server/input/android/android_input_manager.h"
#include "src/server/input/android/android_input_configuration.h"
#include "src/server/input/android/android_input_thread.h"
#include "src/server/input/android/android_input_constants.h"

#include "mir/input/input_channel.h"
#include "mir/input/session_target.h"
#include "mir/input/surface_target.h"

#include "mir_test/fake_shared.h"
#include "mir_test_doubles/mock_viewable_area.h"
#include "mir_test_doubles/stub_session_target.h"
#include "mir_test_doubles/stub_surface_target.h"

#include <InputDispatcher.h>
#include <InputListener.h>
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

// Mock objects
namespace
{

struct MockInputConfiguration : public mia::InputConfiguration
{
    MOCK_METHOD0(the_event_hub, droidinput::sp<droidinput::EventHubInterface>());
    MOCK_METHOD0(the_dispatcher, droidinput::sp<droidinput::InputDispatcherInterface>());
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

        const geom::Rectangle default_view_area =
            geom::Rectangle{geom::Point(),
                            geom::Size{geom::Width(1600), geom::Height(1400)}};


        ON_CALL(view_area, view_area())
            .WillByDefault(Return(default_view_area));

        event_hub = new MockEventHub();
        dispatcher = new MockInputDispatcher();
        dispatcher_thread = std::make_shared<MockInputThread>();
        reader_thread = std::make_shared<MockInputThread>();

        ON_CALL(config, the_event_hub()).WillByDefault(Return(event_hub));
        ON_CALL(config, the_dispatcher()).WillByDefault(Return(dispatcher));
        ON_CALL(config, the_reader_thread()).WillByDefault(Return(reader_thread));
        ON_CALL(config, the_dispatcher_thread()).WillByDefault(Return(dispatcher_thread));
    }
    mtd::MockViewableArea view_area;

    testing::NiceMock<MockInputConfiguration> config;
    droidinput::sp<MockEventHub> event_hub;
    droidinput::sp<MockInputDispatcher> dispatcher;
    std::shared_ptr<MockInputThread> dispatcher_thread;
    std::shared_ptr<MockInputThread> reader_thread;
};

}

TEST_F(AndroidInputManagerSetup, takes_input_setup_from_configuration)
{
    using namespace ::testing;
    
    EXPECT_CALL(config, the_event_hub()).Times(1);
    EXPECT_CALL(config, the_dispatcher()).Times(1);
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
        EXPECT_CALL(*event_hub, wake());
        EXPECT_CALL(*reader_thread, join());
    }

    mia::InputManager manager(mt::fake_shared(config));

    manager.start();
    manager.stop();
}

TEST_F(AndroidInputManagerSetup, manager_returns_input_channel_with_fds)
{
    mia::InputManager manager(mt::fake_shared(config));
    
    auto package = manager.make_input_channel();
    EXPECT_GT(package->client_fd(), 0);
    EXPECT_GT(package->server_fd(), 0);
}

namespace
{

static bool
application_handle_matches_session(droidinput::sp<droidinput::InputApplicationHandle> const& handle,
                                   std::shared_ptr<mi::SessionTarget> const& session)
{
   if (handle->getName() != droidinput::String8(session->name().c_str()))
        return false;
   return true;
}

static bool
window_handle_matches_session_and_surface(droidinput::sp<droidinput::InputWindowHandle> const& handle,
                                          std::shared_ptr<mi::SessionTarget> const& session,
                                          std::shared_ptr<mi::SurfaceTarget> const& surface)
{
    if (!application_handle_matches_session(handle->inputApplicationHandle, session))
        return false;
    if (handle->getInputChannel()->getFd() != surface->server_input_fd())
        return false;
    return true;
}

MATCHER_P2(WindowHandleFor, session, surface, "")
{
    return window_handle_matches_session_and_surface(arg, session, surface);
}

MATCHER_P(ApplicationHandleFor, session, "")
{
    return application_handle_matches_session(arg, session);
}

MATCHER_P2(VectorContainingWindowHandleFor, session, surface, "")
{
    auto i = arg.size();
    for (i = 0; i < arg.size(); i++)
    {
        if (window_handle_matches_session_and_surface(arg[i], session, surface))
            return true;
    }
    return false;
}

MATCHER(EmptyVector, "")
{
    return arg.size() == 0;
}

// TODO: It would be nice if it were possible to mock the interface between 
// droidinput::InputChannel and droidinput::InputDispatcher rather than use
// valid fds to allow non-throwing construction of a real input channel.
struct AndroidInputManagerFdSetup : public AndroidInputManagerSetup
{
    void SetUp() override
    {
        test_input_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    }
    void TearDown() override
    {
        close(test_input_fd);
    }
    int test_input_fd;
};

}

TEST_F(AndroidInputManagerFdSetup, set_input_focus)
{
    using namespace ::testing;
    
    auto session = std::make_shared<mtd::StubSessionTarget>();
    auto surface = std::make_shared<mtd::StubSurfaceTarget>(test_input_fd);
    
    EXPECT_CALL(*dispatcher, registerInputChannel(_, WindowHandleFor(session, surface), false)).Times(1)
        .WillOnce(Return(droidinput::OK));
    EXPECT_CALL(*dispatcher, setFocusedApplication(ApplicationHandleFor(session))).Times(1);
    EXPECT_CALL(*dispatcher, setInputWindows(VectorContainingWindowHandleFor(session, surface))).Times(1);
    EXPECT_CALL(*dispatcher, unregisterInputChannel(_)).Times(1);
    EXPECT_CALL(*dispatcher, setInputWindows(EmptyVector())).Times(1);

    mia::InputManager manager(mt::fake_shared(config));

    manager.set_input_focus_to(session, surface);
    manager.set_input_focus_to(session, std::shared_ptr<mi::SurfaceTarget>());
}
