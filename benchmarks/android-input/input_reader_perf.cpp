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
 * Authored by: Daniel d'Andrada <daniel.dandrada@canonical.com>
 */

// android-input
#include <InputReader.h>
#include <android/keycodes.h>

#include <mir/test/fake_event_hub.h>

#include <chrono>
#include <iostream>
#include <memory>

// local
#include "ntrig_input_events.h"

using namespace android;
using mir::input::android::FakeEventHub;
using std::chrono::duration;
using std::chrono::duration_cast;
using std::chrono::steady_clock;
using std::cout;
using std::endl;

// --- FakeInputReaderPolicy ---

class FakeInputReaderPolicy : public InputReaderPolicyInterface {
    InputReaderConfiguration mConfig;
    Vector<InputDeviceInfo> mInputDevices;

public:
    FakeInputReaderPolicy() {}
    virtual ~FakeInputReaderPolicy() { }

    void setDisplayInfo(int32_t displayId, int32_t width, int32_t height, int32_t orientation) {
        // Set the size of both the internal and external display at the same time.
        mConfig.setDisplayInfo(displayId, false /*external*/, width, height, orientation);
        mConfig.setDisplayInfo(displayId, true /*external*/, width, height, orientation);
    }

    const InputReaderConfiguration* getReaderConfiguration() const {
        return &mConfig;
    }

    const Vector<InputDeviceInfo>& getInputDevices() const {
        return mInputDevices;
    }

    void getAssociatedDisplayInfo(InputDeviceIdentifier const& /* identifier */,
        int& out_associated_display_id, bool& out_associated_display_is_external) override {
        out_associated_display_id = 0;
        out_associated_display_is_external = false;
    }

private:
    virtual void getReaderConfiguration(InputReaderConfiguration* outConfig) override {
        *outConfig = mConfig;
    }

    sp<PointerControllerInterface> obtainPointerController(int32_t deviceId) override {
        (void)deviceId;
        return sp<PointerControllerInterface>();
    }

    virtual void notifyInputDevicesChanged(const Vector<InputDeviceInfo>& inputDevices) override {
        mInputDevices = inputDevices;
    }

    sp<KeyCharacterMap> getKeyboardLayoutOverlay(const String8& inputDeviceDescriptor) override {
        (void) inputDeviceDescriptor;
        return NULL;
    }

    String8 getDeviceAlias(const InputDeviceIdentifier& identifier) override {
        (void)identifier;
        return String8();
    }
};

// --- FakeInputListener ---

class InputListener : public android::InputListenerInterface {
public:
    InputListener() {}
    virtual ~InputListener() {}

    void notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args) override
    {
        (void)args;
    }

    void notifyKey(const NotifyKeyArgs* /*args*/) override {}
    void notifyMotion(const NotifyMotionArgs* args) override;
    void notifySwitch(const NotifySwitchArgs* /*args*/) override {}
    void notifyDeviceReset(const NotifyDeviceResetArgs* /*args*/) override {}
};

void InputListener::notifyMotion(const NotifyMotionArgs* args)
{
    (void)args;
}

// --- main, etc ---

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    const int32_t DEVICE_ID = 2;
    const int32_t DISPLAY_ID = 0;

    std::shared_ptr<FakeEventHub> fakeEventHub = std::make_shared<FakeEventHub>();
    fakeEventHub->addDevice(DEVICE_ID, String8("N-Trig MultiTouch"), INPUT_DEVICE_CLASS_TOUCH_MT);
    fakeEventHub->addConfigurationProperty(DEVICE_ID, "touch.deviceType", "touchScreen");
    fakeEventHub->addConfigurationProperty(DEVICE_ID, "device.internal", "1");
    fakeEventHub->addKey(DEVICE_ID, BTN_TOUCH, 0, AKEYCODE_UNKNOWN, 0);
    fakeEventHub->addAbsoluteAxis(DEVICE_ID, ABS_MT_POSITION_X, 0, 9600, 0, 0, 28);
    fakeEventHub->addAbsoluteAxis(DEVICE_ID, ABS_MT_POSITION_Y, 0, 7200, 0, 0, 37);
    fakeEventHub->addAbsoluteAxis(DEVICE_ID, ABS_MT_TOUCH_MAJOR, 0, 9600, 0, 0, 50);
    fakeEventHub->addAbsoluteAxis(DEVICE_ID, ABS_MT_TOUCH_MINOR, 0, 7200, 0, 0, 37);
    fakeEventHub->addAbsoluteAxis(DEVICE_ID, ABS_MT_ORIENTATION, 0, 1, 0, 0, 0);

    std::shared_ptr<FakeInputReaderPolicy> fakePolicy = std::make_shared<FakeInputReaderPolicy>();
    fakePolicy->setDisplayInfo(DISPLAY_ID, 1024, 768, DISPLAY_ORIENTATION_0);

    std::shared_ptr<InputListener> listener = std::make_shared<InputListener>();

    int32_t total_duration = 0;
    int32_t execution_number;
    const int32_t execution_count = 10;
    for (execution_number = 1; execution_number <= execution_count; ++execution_number) {
        size_t i = 0;
        while (gInputEvents[i].nsecs > 0)
        {
            fakeEventHub->synthesize_event(std::chrono::nanoseconds(gInputEvents[i].nsecs), DEVICE_ID,
                                           gInputEvents[i].type,
                                           gInputEvents[i].code,
                                           gInputEvents[i].value);
            ++i;
        }

        sp<InputReader> inputReader = new InputReader(fakeEventHub, fakePolicy, listener);

        steady_clock::time_point t0 = steady_clock::now();
        while (fakeEventHub->eventsQueueSize() > 0)
        {
            inputReader->loopOnce();
        }
        steady_clock::time_point t1 = steady_clock::now();

        auto time_span = duration_cast<std::chrono::milliseconds>(t1 - t0);

        cout << "Run " << execution_number << ": Took " << time_span.count()
            << " milliseconds to process all input events." << endl;

        total_duration += time_span.count();
    }

    int32_t average_duration = total_duration / execution_count;
    cout << "Average: " << average_duration << " milliseconds." << endl;

    return 0;
}
