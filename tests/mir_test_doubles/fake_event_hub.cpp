#include "mir_test/fake_event_hub.h"

#include <androidfw/Keyboard.h>

using droidinput::AxisInfo;
using droidinput::InputDeviceIdentifier;
using droidinput::PropertyMap;
using droidinput::Vector;
using droidinput::String8;
using droidinput::RawAbsoluteAxisInfo;
using droidinput::RawEvent;
using droidinput::sp;
using droidinput::status_t;
using droidinput::KeyCharacterMap;
using droidinput::VirtualKeyDefinition;

namespace mi = mir::input;
namespace mia = mir::input::android;
namespace mis = mir::input::synthesis;

mia::FakeEventHub::FakeEventHub()
{
    keymap.loadGenericMaps();
}

mia::FakeEventHub::~FakeEventHub()
{
}

uint32_t mia::FakeEventHub::getDeviceClasses(int32_t deviceId) const
{
    if (deviceId == BuiltInKeyboardID)
    {
        return droidinput::INPUT_DEVICE_CLASS_KEYBOARD;
    }
    else if (deviceId == BuiltInCursorID)
    {
        return droidinput::INPUT_DEVICE_CLASS_CURSOR;
    }

    auto fake_device_iterator = device_from_id.find(deviceId);

    if (fake_device_iterator != device_from_id.end())
    {
        return fake_device_iterator->second.classes;
    }
    else
    {
        return 0;
    }
}

InputDeviceIdentifier mia::FakeEventHub::getDeviceIdentifier(int32_t deviceId) const
{
    auto fake_device_iterator = device_from_id.find(deviceId);

    if (fake_device_iterator != device_from_id.end())
    {
        return fake_device_iterator->second.identifier;
    }
    else
    {
        return InputDeviceIdentifier();
    }
}

void mia::FakeEventHub::getConfiguration(int32_t deviceId, PropertyMap* outConfiguration) const
{
    (void)deviceId;
    (void)outConfiguration;
}

status_t mia::FakeEventHub::getAbsoluteAxisInfo(int32_t deviceId, int axis,
        RawAbsoluteAxisInfo* outAxisInfo) const
{
    (void)deviceId;
    (void)axis;
    (void)outAxisInfo;
    return -1;
}

bool mia::FakeEventHub::hasRelativeAxis(int32_t deviceId, int axis) const
{
    (void)deviceId;
    (void)axis;
    return false;
}

bool mia::FakeEventHub::hasInputProperty(int32_t deviceId, int property) const
{
    auto fake_device_iterator = device_from_id.find(deviceId);

    if (fake_device_iterator != device_from_id.end())
    {
        auto property_iterator =
            fake_device_iterator->second.input_properties.find(property);

        if (property_iterator != fake_device_iterator->second.input_properties.end())
        {
            return property_iterator->second;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

status_t mia::FakeEventHub::mapKey(int32_t deviceId, int32_t scanCode, int32_t usageCode,
                              int32_t* outKeycode, uint32_t* outFlags) const
{
    (void)deviceId;
    (void)usageCode;
    keymap.keyLayoutMap->mapKey(scanCode, usageCode, outKeycode, outFlags);
    return droidinput::OK;
}

status_t mia::FakeEventHub::mapAxis(int32_t deviceId, int32_t scanCode,
                               AxisInfo* outAxisInfo) const
{
    (void)deviceId;
    (void)scanCode;
    (void)outAxisInfo;
    return -1;
}

void mia::FakeEventHub::setExcludedDevices(const Vector<String8>& devices)
{
    (void)devices;
}

size_t mia::FakeEventHub::getEvents(int timeoutMillis, RawEvent* buffer, size_t bufferSize)
{
    size_t num_events_obtained = 0;
    (void) timeoutMillis;
    {
        std::lock_guard<std::mutex> lg(guard);
        for (size_t i = 0; i < bufferSize && events_available.size() > 0; ++i)
        {
            buffer[i] = events_available.front();
            events_available.pop_front();
            ++num_events_obtained;
        }
    }

    /* Yield to prevent spinning, which causes long test times under valgrind */
    std::this_thread::yield();

    return num_events_obtained;
}

int32_t mia::FakeEventHub::getScanCodeState(int32_t deviceId, int32_t scanCode) const
{
    (void)deviceId;
    (void)scanCode;
    return AKEY_STATE_UNKNOWN;
}

int32_t mia::FakeEventHub::getKeyCodeState(int32_t deviceId, int32_t keyCode) const
{
    (void)deviceId;
    (void)keyCode;
    return AKEY_STATE_UNKNOWN;
}

int32_t mia::FakeEventHub::getSwitchState(int32_t deviceId, int32_t sw) const
{
    (void)deviceId;
    (void)sw;
    return AKEY_STATE_UNKNOWN;
}

status_t mia::FakeEventHub::getAbsoluteAxisValue(int32_t deviceId, int32_t axis,
        int32_t* outValue) const
{
    (void)deviceId;
    (void)axis;
    (void)outValue;
    return -1;
}

bool mia::FakeEventHub::markSupportedKeyCodes(int32_t deviceId, size_t numCodes,
        const int32_t* keyCodes,
        uint8_t* outFlags) const
{
    (void)deviceId;
    (void)numCodes;
    (void)keyCodes;
    (void)outFlags;
    return true;
}

bool mia::FakeEventHub::hasScanCode(int32_t deviceId, int32_t scanCode) const
{
    (void)deviceId;
    (void)scanCode;
    return false;
}

bool mia::FakeEventHub::hasLed(int32_t deviceId, int32_t led) const
{
    (void)deviceId;
    (void)led;
    return false;
}

void mia::FakeEventHub::setLedState(int32_t deviceId, int32_t led, bool on)
{
    (void)deviceId;
    (void)led;
    (void)on;
}

void mia::FakeEventHub::getVirtualKeyDefinitions(int32_t deviceId,
        Vector<VirtualKeyDefinition>& outVirtualKeys) const
{
    (void)deviceId;
    (void)outVirtualKeys;
}

sp<KeyCharacterMap> mia::FakeEventHub::getKeyCharacterMap(int32_t deviceId) const
{
    (void)deviceId;
    return sp<KeyCharacterMap>();
}

bool mia::FakeEventHub::setKeyboardLayoutOverlay(int32_t deviceId,
        const sp<KeyCharacterMap>& map)
{
    (void)deviceId;
    (void)map;
    return true;
}

void mia::FakeEventHub::vibrate(int32_t deviceId, nsecs_t duration)
{
    (void)deviceId;
    (void)duration;
}

void mia::FakeEventHub::cancelVibrate(int32_t deviceId)
{
    (void)deviceId;
}

void mia::FakeEventHub::requestReopenDevices()
{
}

void mia::FakeEventHub::wake()
{
}

void mia::FakeEventHub::dump(droidinput::String8& dump)
{
    (void)dump;
}

void mia::FakeEventHub::monitor()
{
}

void mia::FakeEventHub::synthesize_builtin_keyboard_added()
{
    RawEvent event;
    event.when = 0;
    event.deviceId = BuiltInKeyboardID;
    event.type = EventHubInterface::DEVICE_ADDED;

    std::lock_guard<std::mutex> lg(guard);
    events_available.push_back(event);
}

void mia::FakeEventHub::synthesize_builtin_cursor_added()
{
    RawEvent event;
    event.when = 0;
    event.deviceId = BuiltInCursorID;
    event.type = EventHubInterface::DEVICE_ADDED;

    std::lock_guard<std::mutex> lg(guard);
    events_available.push_back(event);
}

void mia::FakeEventHub::synthesize_device_scan_complete()
{
    RawEvent event;
    event.when = 0;
    event.type = EventHubInterface::FINISHED_DEVICE_SCAN;

    std::lock_guard<std::mutex> lg(guard);
    events_available.push_back(event);
}

void mia::FakeEventHub::synthesize_event(const mis::KeyParameters &parameters)
{
    RawEvent event;
    event.when = 0;
    event.type = EV_KEY;
    event.code = parameters.scancode;

    if (parameters.device_id)
        event.deviceId = parameters.device_id;
    else
        event.deviceId = BuiltInKeyboardID;

    if (parameters.action == mis::EventAction::Down)
        event.value = 1;
    else
        event.value = 0;

    std::lock_guard<std::mutex> lg(guard);
    events_available.push_back(event);
}

void mia::FakeEventHub::synthesize_event(const mis::ButtonParameters &parameters)
{
    RawEvent event;
    event.when = 0;
    event.type = EV_KEY;
    event.code = parameters.button;

    if (parameters.device_id)
        event.deviceId = parameters.device_id;
    else
        event.deviceId = BuiltInCursorID;

    if (parameters.action == mis::EventAction::Down)
        event.value = 1;
    else
        event.value = 0;

    std::lock_guard<std::mutex> lg(guard);
    events_available.push_back(event);

    // Cursor button events require a sync as per droidinput::CursorInputMapper::process
    event.type = EV_SYN;
    event.code = SYN_REPORT;
    events_available.push_back(event);
}

void mia::FakeEventHub::synthesize_event(const mis::MotionParameters &parameters)
{
    RawEvent event;
    event.when = 0;
    event.type = EV_REL;
    if (parameters.device_id)
        event.deviceId = parameters.device_id;
    else
        event.deviceId = BuiltInCursorID;

    std::lock_guard<std::mutex> lg(guard);
    event.code = REL_X;
    event.value = parameters.rel_x;
    events_available.push_back(event);

    event.code = REL_Y;
    event.value = parameters.rel_y;
    events_available.push_back(event);

    // Cursor motion events require a sync as per droidinput::CursorInputMapper::process
    event.type = EV_SYN;
    event.code = SYN_REPORT;
    events_available.push_back(event);
}
