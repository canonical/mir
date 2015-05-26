/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@gmail.com>
 */

#ifndef MIR_TEST_DOUBLES_MOCK_EVENT_HUB_H_
#define MIR_TEST_DOUBLES_MOCK_EVENT_HUB_H_

#include <EventHub.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace droidinput = android;
namespace mir
{
namespace test
{
namespace doubles
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
    MOCK_METHOD2(getEvents, size_t(droidinput::RawEvent*, size_t));
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
    MOCK_METHOD2(vibrate, void(int32_t, std::chrono::nanoseconds));
    MOCK_METHOD1(cancelVibrate, void(int32_t));
    MOCK_METHOD0(requestReopenDevices, void());
    MOCK_METHOD0(wake, void());
    MOCK_METHOD1(wakeIn, void(int32_t));
    MOCK_METHOD1(dump, void(droidinput::String8&));
    MOCK_METHOD0(monitor, void());
    MOCK_METHOD0(flush, void());
    MOCK_METHOD0(fd, mir::Fd());
};
}
}
}
#endif
