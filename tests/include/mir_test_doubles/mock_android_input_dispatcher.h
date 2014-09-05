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

#ifndef MIR_TEST_DOUBLES_MOCK_ANDROID_INPUT_DISPATCHER_H_
#define MIR_TEST_DOUBLES_MOCK_ANDROID_INPUT_DISPATCHER_H_

#include <InputDispatcher.h>
#include <gmock/gmock.h>

namespace droidinput = android;

namespace mir
{
namespace test
{
namespace doubles
{

struct MockAndroidInputDispatcher : public droidinput::InputDispatcherInterface
{
    // droidinput::InputDispatcher interface
    MOCK_METHOD1(setInputEnumerator, void(droidinput::sp<droidinput::InputEnumerator> const&));
    MOCK_METHOD1(dump, void(droidinput::String8&));
    MOCK_METHOD0(monitor, void());
    MOCK_METHOD0(dispatchOnce, void());
    MOCK_METHOD6(injectInputEvent, int32_t(droidinput::InputEvent const*, int32_t, int32_t, int32_t, int32_t, uint32_t));
    MOCK_METHOD1(setFocusedApplication, void(droidinput::sp<droidinput::InputApplicationHandle> const&));
    MOCK_METHOD2(setInputDispatchMode, void(bool, bool));
    MOCK_METHOD1(setInputFilterEnabled, void(bool));
    MOCK_METHOD2(transferTouchFocus, bool(droidinput::sp<droidinput::InputChannel> const&, droidinput::sp<droidinput::InputChannel> const&));
    MOCK_METHOD3(registerInputChannel, droidinput::status_t(droidinput::sp<droidinput::InputChannel> const&, droidinput::sp<droidinput::InputWindowHandle> const&, bool));
    MOCK_METHOD1(unregisterInputChannel, droidinput::status_t(droidinput::sp<droidinput::InputChannel> const&));

    MOCK_METHOD1(setKeyboardFocus, void(droidinput::sp<droidinput::InputWindowHandle> const&));
    MOCK_METHOD1(notifyWindowRemoved, void(droidinput::sp<droidinput::InputWindowHandle> const&));

    // droidinput::InputListener interface
    MOCK_METHOD1(notifyConfigurationChanged, void(droidinput::NotifyConfigurationChangedArgs const*));
    MOCK_METHOD1(notifyKey, void(droidinput::NotifyKeyArgs const*));
    MOCK_METHOD1(notifyMotion, void(droidinput::NotifyMotionArgs const*));
    MOCK_METHOD1(notifySwitch, void(droidinput::NotifySwitchArgs const*));
    MOCK_METHOD1(notifyDeviceReset, void(droidinput::NotifyDeviceResetArgs const*));
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_INPUT_DISPATCHER_H_
