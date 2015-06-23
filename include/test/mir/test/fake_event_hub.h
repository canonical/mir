/*
 * Copyright © 2012-2013 Canonical Ltd.
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
 *              Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

#ifndef MIR_TEST_FAKE_EVENT_HUB_H_
#define MIR_TEST_FAKE_EVENT_HUB_H_

#include "mir/test/event_factory.h"

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
    struct KeyInfo {
        int32_t keyCode;
        uint32_t flags;
    };

    typedef struct FakeDevice
    {
        uint32_t classes;
        droidinput::InputDeviceIdentifier identifier;
        droidinput::PropertyMap configuration;
        droidinput::KeyedVector<int, droidinput::RawAbsoluteAxisInfo> absoluteAxes;
        droidinput::KeyedVector<int, bool> relativeAxes;
        droidinput::KeyedVector<int32_t, int32_t> keyCodeStates;
        droidinput::KeyedVector<int32_t, int32_t> scanCodeStates;
        droidinput::KeyedVector<int32_t, int32_t> switchStates;
        droidinput::KeyedVector<int32_t, int32_t> absoluteAxisValue;
        droidinput::KeyedVector<int32_t, KeyInfo> keysByScanCode;
        droidinput::KeyedVector<int32_t, KeyInfo> keysByUsageCode;
        droidinput::KeyedVector<int32_t, bool> leds;
        droidinput::Vector<droidinput::VirtualKeyDefinition> virtualKeys;
        std::map<int, bool> input_properties;
    } FakeDevice;

    FakeEventHub();
    virtual ~FakeEventHub();

    static const int BuiltInKeyboardID = droidinput::BUILT_IN_KEYBOARD_ID;
    // Any positive int besides BUILT_IN_KEYBOARD_ID (which has
    // special meaning) will do. There is no notion of a builtin
    // cursor device in the android input stack.
    static const int BuiltInCursorID = droidinput::BUILT_IN_KEYBOARD_ID + 1;
    static const int USBTouchscreenID;

    static const int TouchScreenMinAxisValue;
    static const int TouchScreenMaxAxisValue;

    // From EventHubInterface
    uint32_t getDeviceClasses(int32_t deviceId) const override;
    droidinput::InputDeviceIdentifier getDeviceIdentifier(int32_t deviceId) const override;
    void getConfiguration(int32_t deviceId, droidinput::PropertyMap* outConfiguration) const override;
    droidinput::status_t getAbsoluteAxisInfo(int32_t deviceId, int axis,
                                  droidinput::RawAbsoluteAxisInfo* outAxisInfo) const override;
    bool hasRelativeAxis(int32_t deviceId, int axis) const override;
    bool hasInputProperty(int32_t deviceId, int property) const override;
    droidinput::status_t mapKey(int32_t deviceId, int32_t scanCode, int32_t usageCode, int32_t* outKeycode,
            uint32_t* outFlags) const override;
    droidinput::status_t mapAxis(int32_t deviceId, int32_t scanCode,
            droidinput::AxisInfo* outAxisInfo) const override;
    void setExcludedDevices(const droidinput::Vector<droidinput::String8>& devices) override;
    size_t getEvents(droidinput::RawEvent* buffer, size_t bufferSize) override;
    int32_t getScanCodeState(int32_t deviceId, int32_t scanCode) const override;
    int32_t getKeyCodeState(int32_t deviceId, int32_t keyCode) const override;
    int32_t getSwitchState(int32_t deviceId, int32_t sw) const override;
    droidinput::status_t getAbsoluteAxisValue(int32_t deviceId, int32_t axis, int32_t* outValue) const override;
    bool markSupportedKeyCodes(int32_t deviceId, size_t numCodes, const int32_t* keyCodes,
            uint8_t* outFlags) const override;
    bool hasScanCode(int32_t deviceId, int32_t scanCode) const override;
    bool hasLed(int32_t deviceId, int32_t led) const override;
    void setLedState(int32_t deviceId, int32_t led, bool on) override;
    void getVirtualKeyDefinitions(int32_t deviceId,
            droidinput::Vector<droidinput::VirtualKeyDefinition>& outVirtualKeys) const override;
    droidinput::sp<droidinput::KeyCharacterMap> getKeyCharacterMap(int32_t deviceId) const override;
    bool setKeyboardLayoutOverlay(int32_t deviceId,
            const droidinput::sp<droidinput::KeyCharacterMap>& map) override;
    void vibrate(int32_t deviceId, std::chrono::nanoseconds duration) override;
    void cancelVibrate(int32_t deviceId) override;
    void requestReopenDevices() override;
    void wakeIn(int32_t) override;
    void wake() override;
    void wake(droidinput::RawEvent const&);
    void dump(droidinput::String8& dump) override;
    void monitor() override;
    void flush() override;
    mir::Fd fd() override;

    void synthesize_builtin_keyboard_added();
    void synthesize_builtin_cursor_added();
    void synthesize_usb_touchscreen_added();
    void synthesize_device_scan_complete();

    void synthesize_event(synthesis::KeyParameters const& parameters);
    void synthesize_event(synthesis::ButtonParameters const& parameters);
    void synthesize_event(synthesis::MotionParameters const& parameters);
    void synthesize_event(synthesis::TouchParameters const& parameters);
    void synthesize_event(std::chrono::nanoseconds when, int32_t deviceId, int32_t type, int32_t code, int32_t value);

    void addDevice(int32_t deviceId, const std::string& name, uint32_t classes);
    void removeDevice(int32_t deviceId);
    void finishDeviceScan();
    void addConfigurationProperty(int32_t device_id, const std::string& key, const std::string& value);
    void addConfigurationMap(int32_t device_id, const droidinput::PropertyMap* configuration);
    void addAbsoluteAxis(int32_t device_id, int axis, int32_t minValue, int32_t maxValue,
                           int flat, int fuzz, int resolution = 0);
    void addRelativeAxis(int32_t device_id, int32_t axis);
    void setKeyCodeState(int32_t deviceId, int32_t keyCode, int32_t state);
    void setScanCodeState(int32_t deviceId, int32_t scanCode, int32_t state);
    void setSwitchState(int32_t deviceId, int32_t switchCode, int32_t state);
    void setAbsoluteAxisValue(int32_t deviceId, int32_t axis, int32_t value);
    void addKey(int32_t deviceId, int32_t scanCode, int32_t usageCode, int32_t keyCode, uint32_t flags);
    void addLed(int32_t deviceId, int32_t led, bool initialState);
    bool getLedState(int32_t deviceId, int32_t led);
    droidinput::Vector<std::string>& getExcludedDevices();
    void addVirtualKeyDefinition(int32_t deviceId, const droidinput::VirtualKeyDefinition& definition);
    const FakeDevice* getDevice(int32_t deviceId) const;
    FakeDevice* getDevice(int32_t deviceId);
    size_t eventsQueueSize() const;

    void throw_exception_in_next_get_events();

    // list of RawEvents available for consumption via getEvents
    mutable std::mutex guard;
    std::list<droidinput::RawEvent> events_available;


    std::map<int32_t, FakeDevice> device_from_id;

    droidinput::KeyMap keymap;

    droidinput::Vector<droidinput::String8> excluded_devices;

private:
    const KeyInfo* getKey(const FakeDevice* device, int32_t scanCode, int32_t usageCode) const;
    bool throw_in_get_events = false;
    mir::Fd trigger_fd;  // event fd used internally when events are added to the list, with that fd() could be used to
                         // wake an epoll_wait
};
}
}
} // namespace mir

#endif // MIR_TEST_FAKE_EVENT_HUB_H_
