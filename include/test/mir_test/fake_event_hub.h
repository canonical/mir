/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com
 */

#ifndef MIR_TEST_FAKE_EVENT_HUB_H_
#define MIR_TEST_FAKE_EVENT_HUB_H_

#include "mir_test/event_factory.h"

#include <atomic>
#include <mutex>

// from android-input
#include <EventHub.h>

#include <list>
#include <map>

#include <androidfw/Keyboard.h>
namespace droidinput = android;

namespace mir
{
namespace input
{
namespace android
{
// An EventHub implementation that generates fake raw events.
class FakeEventHub : public droidinput::EventHubInterface
{
public:
    FakeEventHub();
    virtual ~FakeEventHub();

    static const int BuiltInKeyboardID = droidinput::BUILT_IN_KEYBOARD_ID;
    // Any positive int besides BUILT_IN_KEYBOARD_ID (which has
    // special meaning) will do. There is no notion of a builtin
    // cursor device in the android input stack.
    static const int BuiltInCursorID = droidinput::BUILT_IN_KEYBOARD_ID + 1;

    virtual uint32_t getDeviceClasses(int32_t deviceId) const;

    virtual droidinput::InputDeviceIdentifier getDeviceIdentifier(int32_t deviceId) const;

    virtual void getConfiguration(int32_t deviceId,
            droidinput::PropertyMap* outConfiguration) const;

    virtual droidinput::status_t getAbsoluteAxisInfo(int32_t deviceId, int axis,
            droidinput::RawAbsoluteAxisInfo* outAxisInfo) const;

    virtual bool hasRelativeAxis(int32_t deviceId, int axis) const;

    virtual bool hasInputProperty(int32_t deviceId, int property) const;

    virtual droidinput::status_t mapKey(int32_t deviceId, int32_t scanCode, int32_t usageCode,
            int32_t* outKeycode, uint32_t* outFlags) const;

    virtual droidinput::status_t mapAxis(int32_t deviceId, int32_t scanCode,
            droidinput::AxisInfo* outAxisInfo) const;

    virtual void setExcludedDevices(const droidinput::Vector<droidinput::String8>& devices);

    virtual size_t getEvents(int timeoutMillis, droidinput::RawEvent* buffer,
            size_t bufferSize);

    virtual int32_t getScanCodeState(int32_t deviceId, int32_t scanCode) const;
    virtual int32_t getKeyCodeState(int32_t deviceId, int32_t keyCode) const;
    virtual int32_t getSwitchState(int32_t deviceId, int32_t sw) const;
    virtual droidinput::status_t getAbsoluteAxisValue(int32_t deviceId, int32_t axis,
                                          int32_t* outValue) const;

    virtual bool markSupportedKeyCodes(int32_t deviceId, size_t numCodes,
            const int32_t* keyCodes, uint8_t* outFlags) const;

    virtual bool hasScanCode(int32_t deviceId, int32_t scanCode) const;
    virtual bool hasLed(int32_t deviceId, int32_t led) const;
    virtual void setLedState(int32_t deviceId, int32_t led, bool on);

    virtual void getVirtualKeyDefinitions(int32_t deviceId,
            droidinput::Vector<droidinput::VirtualKeyDefinition>& outVirtualKeys) const;

    virtual droidinput::sp<droidinput::KeyCharacterMap> getKeyCharacterMap(int32_t deviceId) const;
    virtual bool setKeyboardLayoutOverlay(int32_t deviceId,
                                          const droidinput::sp<droidinput::KeyCharacterMap>& map);

    virtual void vibrate(int32_t deviceId, nsecs_t duration);
    virtual void cancelVibrate(int32_t deviceId);

    virtual void requestReopenDevices();

    virtual void wake();

    virtual void dump(droidinput::String8& dump);

    virtual void monitor();

    void synthesize_builtin_keyboard_added();
    void synthesize_builtin_cursor_added();
    void synthesize_device_scan_complete();

    void synthesize_event(const synthesis::KeyParameters &parameters);
    void synthesize_event(const synthesis::ButtonParameters &parameters);
    void synthesize_event(const synthesis::MotionParameters &parameters);

    // list of RawEvents available for consumption via getEvents
    std::mutex guard;
    std::list<droidinput::RawEvent> events_available;

    typedef struct FakeDevice
    {
        uint32_t classes;
        droidinput::InputDeviceIdentifier identifier;
        std::map<int, bool> input_properties;
    } FakeDevice;

    std::map<int32_t, FakeDevice> device_from_id;

    droidinput::KeyMap keymap;

};
}
}
} // namespace mir

#endif // MIR_TEST_FAKE_EVENT_HUB_H_
