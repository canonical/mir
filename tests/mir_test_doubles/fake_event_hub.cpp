/*
 * Copyright (C) 2012-2013 Canonical Ltd.
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mir/test/fake_event_hub.h"

#include "mir/log.h"
#include "mir/logging/logger.h"

// from android-input
#include <androidfw/Keyboard.h>
#include <std/Errors.h>

#include <thread>
#include <chrono>

#include <sys/eventfd.h>

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
namespace mis = mir::input::synthesis;

using mir::input::android::FakeEventHub;
using namespace android;

namespace {
    // An arbitrary time value.
    constexpr const std::chrono::nanoseconds arbitrary_time = std::chrono::nanoseconds(1234);
} // anonymous namespace

int const FakeEventHub::USBTouchscreenID = droidinput::BUILT_IN_KEYBOARD_ID + 2;
int const FakeEventHub::TouchScreenMinAxisValue = 0;
int const FakeEventHub::TouchScreenMaxAxisValue = 100;

FakeEventHub::FakeEventHub()
    : trigger_fd{eventfd(0, EFD_CLOEXEC|EFD_NONBLOCK)}
{
    keymap.loadGenericMaps();
}

FakeEventHub::~FakeEventHub()
{
}

uint32_t FakeEventHub::getDeviceClasses(int32_t deviceId) const
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

InputDeviceIdentifier FakeEventHub::getDeviceIdentifier(int32_t deviceId) const
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

void FakeEventHub::getConfiguration(int32_t deviceId, PropertyMap* outConfiguration) const
{
    auto device_iterator = device_from_id.find(deviceId);
    if (device_iterator != device_from_id.end())
    {
        *outConfiguration = device_iterator->second.configuration;
    }
}

status_t FakeEventHub::getAbsoluteAxisInfo(int32_t deviceId, int axis,
        RawAbsoluteAxisInfo* outAxisInfo) const
{
    outAxisInfo->clear();
    const FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        ssize_t index = device->absoluteAxes.indexOfKey(axis);
        if (index >= 0)
        {
            *outAxisInfo = device->absoluteAxes.valueAt(index);
            return OK;
        }
    }
    return -1;
}

bool FakeEventHub::hasRelativeAxis(int32_t deviceId, int axis) const
{
    const FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        return device->relativeAxes.indexOfKey(axis) >= 0;
    }
    return false;
}

bool FakeEventHub::hasInputProperty(int32_t deviceId, int property) const
{
    const FakeDevice* device = getDevice(deviceId);

    if (device)
    {
        auto property_iterator = device->input_properties.find(property);

        if (property_iterator != device->input_properties.end())
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

status_t FakeEventHub::mapKey(int32_t deviceId, int32_t scanCode, int32_t usageCode,
                              int32_t* outKeycode, uint32_t* outFlags) const
{
    const FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        const KeyInfo* key = getKey(device, scanCode, usageCode);
        if (key)
        {
            if (outKeycode)
            {
                *outKeycode = key->keyCode;
            }
            if (outFlags)
            {
                *outFlags = key->flags;
            }
            return OK;
        }
        return NAME_NOT_FOUND;
    }
    else
    {
        keymap.keyLayoutMap->mapKey(scanCode, usageCode, outKeycode, outFlags);
        return droidinput::OK;
    }
}

const FakeEventHub::KeyInfo* FakeEventHub::getKey(const FakeDevice* device,
                                                  int32_t scanCode, int32_t usageCode) const
{
    if (usageCode)
    {
        ssize_t index = device->keysByUsageCode.indexOfKey(usageCode);
        if (index >= 0)
        {
            return &device->keysByUsageCode.valueAt(index);
        }
    }
    if (scanCode)
    {
        ssize_t index = device->keysByScanCode.indexOfKey(scanCode);
        if (index >= 0)
        {
            return &device->keysByScanCode.valueAt(index);
        }
    }
    return NULL;
}

status_t FakeEventHub::mapAxis(int32_t deviceId, int32_t scanCode,
                               AxisInfo* outAxisInfo) const
{
    (void)deviceId;
    (void)scanCode;
    (void)outAxisInfo;
    return NAME_NOT_FOUND;
}

void FakeEventHub::setExcludedDevices(const Vector<String8>& devices)
{
    excluded_devices = devices;
}

size_t FakeEventHub::getEvents(RawEvent* buffer, size_t bufferSize)
{
    size_t num_events_obtained = 0;
    {
        std::lock_guard<std::mutex> lg(guard);
        uint64_t dummy;
        if (sizeof dummy != read(trigger_fd, &dummy, sizeof dummy))
            mir::log(mir::logging::Severity::debug, "FakeEventHub", "No event trigger to consume");

        if (throw_in_get_events)
            throw std::runtime_error("FakeEventHub::getEvents() exception");

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

int32_t FakeEventHub::getScanCodeState(int32_t deviceId, int32_t scanCode) const
{
    const FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        ssize_t index = device->scanCodeStates.indexOfKey(scanCode);
        if (index >= 0)
        {
            return device->scanCodeStates.valueAt(index);
        }
    }
    return AKEY_STATE_UNKNOWN;
}

int32_t FakeEventHub::getKeyCodeState(int32_t deviceId, int32_t keyCode) const
{
    const FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        ssize_t index = device->keyCodeStates.indexOfKey(keyCode);
        if (index >= 0)
        {
            return device->keyCodeStates.valueAt(index);
        }
    }
    return AKEY_STATE_UNKNOWN;
}

int32_t FakeEventHub::getSwitchState(int32_t deviceId, int32_t sw) const
{
    const FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        ssize_t index = device->switchStates.indexOfKey(sw);
        if (index >= 0)
        {
            return device->switchStates.valueAt(index);
        }
    }
    return AKEY_STATE_UNKNOWN;
}

status_t FakeEventHub::getAbsoluteAxisValue(int32_t deviceId, int32_t axis,
        int32_t* outValue) const
{
    const FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        ssize_t index = device->absoluteAxisValue.indexOfKey(axis);
        if (index >= 0)
        {
            *outValue = device->absoluteAxisValue.valueAt(index);
            return OK;
        }
    }
    *outValue = 0;
    return -1;
}

bool FakeEventHub::markSupportedKeyCodes(int32_t deviceId, size_t numCodes,
        const int32_t* keyCodes,
        uint8_t* outFlags) const
{
    bool result = false;
    const FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        for (size_t i = 0; i < numCodes; i++)
        {
            for (size_t j = 0; j < device->keysByScanCode.size(); j++)
            {
                if (keyCodes[i] == device->keysByScanCode.valueAt(j).keyCode)
                {
                    outFlags[i] = 1;
                    result = true;
                }
            }
            for (size_t j = 0; j < device->keysByUsageCode.size(); j++)
            {
                if (keyCodes[i] == device->keysByUsageCode.valueAt(j).keyCode)
                {
                    outFlags[i] = 1;
                    result = true;
                }
            }
        }
    }
    return result;
}

bool FakeEventHub::hasScanCode(int32_t deviceId, int32_t scanCode) const
{
    const FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        ssize_t index = device->keysByScanCode.indexOfKey(scanCode);
        return index >= 0;
    }
    return false;
}

bool FakeEventHub::hasLed(int32_t deviceId, int32_t led) const
{
    const FakeDevice* device = getDevice(deviceId);
    return device && device->leds.indexOfKey(led) >= 0;
}

void FakeEventHub::setLedState(int32_t deviceId, int32_t led, bool on)
{
    FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        ssize_t index = device->leds.indexOfKey(led);
        if (index >= 0)
        {
            device->leds.replaceValueAt(led, on);
        }
    }
}

void FakeEventHub::getVirtualKeyDefinitions(int32_t deviceId,
        Vector<VirtualKeyDefinition>& outVirtualKeys) const
{
    outVirtualKeys.clear();

    const FakeDevice* device = getDevice(deviceId);
    if (device)
    {
        outVirtualKeys.appendVector(device->virtualKeys);
    }
}

sp<KeyCharacterMap> FakeEventHub::getKeyCharacterMap(int32_t deviceId) const
{
    (void)deviceId;
    return sp<KeyCharacterMap>();
}

bool FakeEventHub::setKeyboardLayoutOverlay(int32_t deviceId,
        const sp<KeyCharacterMap>& map)
{
    (void)deviceId;
    (void)map;
    return true;
}

void FakeEventHub::vibrate(int32_t deviceId, std::chrono::nanoseconds duration)
{
    (void)deviceId;
    (void)duration;
}

void FakeEventHub::cancelVibrate(int32_t deviceId)
{
    (void)deviceId;
}

void FakeEventHub::requestReopenDevices()
{
}

void FakeEventHub::wakeIn(int32_t)
{
}

void FakeEventHub::wake()
{
    uint64_t one{1};
    if (sizeof one != write(trigger_fd, &one, sizeof one))
        BOOST_THROW_EXCEPTION((std::system_error{errno,
                               std::system_category(),
                               "Failed to wake fake event hub"}));
}

void FakeEventHub::wake(droidinput::RawEvent const& event)
{
    std::lock_guard<std::mutex> lg(guard);
    events_available.push_back(event);
    wake();
}

void FakeEventHub::dump(droidinput::String8& dump)
{
    (void)dump;
}

void FakeEventHub::monitor()
{
}

void FakeEventHub::flush()
{
}

mir::Fd FakeEventHub::fd()
{
    return trigger_fd;
}

void FakeEventHub::synthesize_builtin_keyboard_added()
{
    RawEvent event;
    event.when = std::chrono::nanoseconds(0);
    event.deviceId = BuiltInKeyboardID;
    event.type = EventHubInterface::DEVICE_ADDED;

    wake(event);
}

void FakeEventHub::synthesize_builtin_cursor_added()
{
    RawEvent event;
    event.when = std::chrono::nanoseconds(0);
    event.deviceId = BuiltInCursorID;
    event.type = EventHubInterface::DEVICE_ADDED;

    wake(event);
}

void FakeEventHub::synthesize_usb_touchscreen_added()
{
    FakeDevice device;
    device.classes = INPUT_DEVICE_CLASS_TOUCH | INPUT_DEVICE_CLASS_EXTERNAL;
    device.input_properties[INPUT_PROP_DIRECT] = true;
    device.identifier.bus = BUS_USB;

    RawAbsoluteAxisInfo axis_info;
    axis_info.valid = true;
    axis_info.minValue = TouchScreenMinAxisValue;
    axis_info.maxValue = TouchScreenMaxAxisValue;
    axis_info.flat = 0;
    axis_info.fuzz = 0;
    axis_info.resolution = 1;
    device.absoluteAxes.add(ABS_X, axis_info);
    device.absoluteAxes.add(ABS_Y, axis_info);

    device_from_id.insert(std::pair<int32_t, FakeDevice>(USBTouchscreenID, device));
    
    RawEvent event;
    event.when = std::chrono::nanoseconds(0);
    event.deviceId = USBTouchscreenID;
    event.type = EventHubInterface::DEVICE_ADDED;
    
    std::lock_guard<std::mutex> lg(guard);
    events_available.push_back(event);
    wake();
}

void FakeEventHub::synthesize_device_scan_complete()
{
    RawEvent event;
    event.when = std::chrono::nanoseconds(0);
    event.type = EventHubInterface::FINISHED_DEVICE_SCAN;

    wake(event);
}

void FakeEventHub::synthesize_event(const mis::KeyParameters &parameters)
{
    RawEvent event;
    event.when = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
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

    wake(event);
}

void FakeEventHub::synthesize_event(const mis::ButtonParameters &parameters)
{
    RawEvent event;
    event.when = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
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
    wake();
}

void FakeEventHub::synthesize_event(const mis::MotionParameters &parameters)
{
    RawEvent event;
    event.when = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
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
    wake();
}

void FakeEventHub::synthesize_event(const mis::TouchParameters &parameters)
{
    RawEvent event;
    event.when = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch());
    event.type = EV_ABS;
    if (parameters.device_id)
        event.deviceId = parameters.device_id;
    else
        event.deviceId = USBTouchscreenID;

    std::lock_guard<std::mutex> lg(guard);
    event.code = ABS_X;
    event.value = parameters.abs_x;
    events_available.push_back(event);

    event.code = ABS_Y;
    event.value = parameters.abs_y;
    events_available.push_back(event);

    if (parameters.action != mis::TouchParameters::Action::Move)
    {
        event.type = EV_KEY;
        event.code = BTN_TOUCH;
        event.value = (parameters.action == mis::TouchParameters::Action::Tap);
        events_available.push_back(event);
    }

    // Cursor motion events require a sync as per droidinput::CursorInputMapper::process
    event.type = EV_SYN;
    event.code = SYN_REPORT;
    events_available.push_back(event);
    wake();
}

void FakeEventHub::synthesize_event(std::chrono::nanoseconds when, int32_t device_id, int32_t type, int32_t code, int32_t value)
{
    RawEvent event;
    event.when = when;
    event.deviceId = device_id;
    event.type = type;
    event.code = code;
    event.value = value;

    wake(event);

    if (type == EV_ABS)
    {
        setAbsoluteAxisValue(device_id, code, value);
    }
}

void FakeEventHub::addDevice(int32_t deviceId, const std::string& name, uint32_t classes)
{
    FakeDevice device;
    device.classes = classes;
    device.identifier.name = name;
    device_from_id.insert(std::pair<int32_t, FakeDevice>(deviceId, device));

    synthesize_event(arbitrary_time, deviceId, EventHubInterface::DEVICE_ADDED, 0, 0);
}

void FakeEventHub::removeDevice(int32_t device_id)
{
    device_from_id.erase(device_id);

    synthesize_event(arbitrary_time, device_id, EventHubInterface::DEVICE_REMOVED, 0, 0);
}

void FakeEventHub::finishDeviceScan()
{
    synthesize_event(arbitrary_time, 0, EventHubInterface::FINISHED_DEVICE_SCAN, 0, 0);
}

void FakeEventHub::addConfigurationProperty(int32_t device_id, const std::string& key, const std::string& value)
{
    FakeDevice& device = device_from_id.at(device_id);
    device.configuration.addProperty(key, value);
}

void FakeEventHub::addConfigurationMap(int32_t device_id, const PropertyMap* configuration)
{
    FakeDevice& device = device_from_id.at(device_id);
    device.configuration.addAll(configuration);
}

void FakeEventHub::addAbsoluteAxis(int32_t device_id, int axis,
        int32_t minValue, int32_t maxValue, int flat, int fuzz, int resolution)
{
    FakeDevice& device = device_from_id.at(device_id);

    RawAbsoluteAxisInfo info;
    info.valid = true;
    info.minValue = minValue;
    info.maxValue = maxValue;
    info.flat = flat;
    info.fuzz = fuzz;
    info.resolution = resolution;
    device.absoluteAxes.add(axis, info);
}

void FakeEventHub::addRelativeAxis(int32_t device_id, int32_t axis)
{
    FakeDevice& device = device_from_id.at(device_id);
    device.relativeAxes.add(axis, true);
}

void FakeEventHub::setKeyCodeState(int32_t deviceId, int32_t keyCode, int32_t state)
{
    FakeDevice& device = device_from_id.at(deviceId);
    device.keyCodeStates.replaceValueFor(keyCode, state);
}

void FakeEventHub::setScanCodeState(int32_t deviceId, int32_t scanCode, int32_t state)
{
    FakeDevice& device = device_from_id.at(deviceId);
    device.scanCodeStates.replaceValueFor(scanCode, state);
}

void FakeEventHub::setSwitchState(int32_t deviceId, int32_t switchCode, int32_t state)
{
    FakeDevice& device = device_from_id.at(deviceId);
    device.switchStates.replaceValueFor(switchCode, state);
}

void FakeEventHub::setAbsoluteAxisValue(int32_t deviceId, int32_t axis, int32_t value)
{
    FakeDevice& device = device_from_id.at(deviceId);
    device.absoluteAxisValue.replaceValueFor(axis, value);
}

void FakeEventHub::addKey(int32_t deviceId, int32_t scanCode, int32_t usageCode,
                          int32_t keyCode, uint32_t flags)
{
    FakeDevice& device = device_from_id.at(deviceId);
    KeyInfo info;
    info.keyCode = keyCode;
    info.flags = flags;
    if (scanCode)
    {
        device.keysByScanCode.add(scanCode, info);
    }
    if (usageCode)
    {
        device.keysByUsageCode.add(usageCode, info);
    }
}

void FakeEventHub::addLed(int32_t deviceId, int32_t led, bool initialState)
{
    FakeDevice& device = device_from_id.at(deviceId);
    device.leds.add(led, initialState);
}

bool FakeEventHub::getLedState(int32_t deviceId, int32_t led)
{
    FakeDevice& device = device_from_id.at(deviceId);
    return device.leds.valueFor(led);
}

Vector<std::string>& FakeEventHub::getExcludedDevices()
{
    return excluded_devices;
}

void FakeEventHub::addVirtualKeyDefinition(int32_t deviceId, const VirtualKeyDefinition& definition)
{
    FakeDevice& device = device_from_id.at(deviceId);
    device.virtualKeys.push(definition);
}

FakeEventHub::FakeDevice* FakeEventHub::getDevice(int32_t deviceId)
{
    auto fake_device_iterator = device_from_id.find(deviceId);

    if (fake_device_iterator != device_from_id.end())
    {
        return &(fake_device_iterator->second);
    }
    else
    {
        return nullptr;
    }
}

const FakeEventHub::FakeDevice* FakeEventHub::getDevice(int32_t deviceId) const
{
    auto fake_device_iterator = device_from_id.find(deviceId);

    if (fake_device_iterator != device_from_id.end())
    {
        return &(fake_device_iterator->second);
    }
    else
    {
        return nullptr;
    }
}

size_t FakeEventHub::eventsQueueSize() const
{
    std::lock_guard<std::mutex> lg(guard);
    return events_available.size();
}

void FakeEventHub::throw_exception_in_next_get_events()
{
    std::lock_guard<std::mutex> lg(guard);
    throw_in_get_events = true;
    wake();
}
