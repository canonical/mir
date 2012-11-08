#include <EventHub.h>
#include <InputListener.h>
#include <InputReader.h>

#include <iostream>

using namespace android;

/******************************************************************************
 * InputListener
 ******************************************************************************/

class InputListener : public android::InputListenerInterface {
public:
    InputListener();
    virtual ~InputListener();

    virtual void notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args);
    virtual void notifyKey(const NotifyKeyArgs* args);
    virtual void notifyMotion(const NotifyMotionArgs* args);
    virtual void notifySwitch(const NotifySwitchArgs* args);
    virtual void notifyDeviceReset(const NotifyDeviceResetArgs* args);
};

InputListener::InputListener()
{
}

InputListener::~InputListener()
{
}

void InputListener::notifyConfigurationChanged(const NotifyConfigurationChangedArgs* args)
{
}

void InputListener::notifyKey(const NotifyKeyArgs* args)
{
    std::cout << "key" << std::endl;
}

void InputListener::notifyMotion(const NotifyMotionArgs* args)
{
    std::cout << "motion" << std::endl;
}

void InputListener::notifySwitch(const NotifySwitchArgs* args)
{
}

void InputListener::notifyDeviceReset(const NotifyDeviceResetArgs* args)
{
}

/******************************************************************************
 * InputReaderPolicy
 ******************************************************************************/

class InputReaderPolicy : public android::InputReaderPolicyInterface {
public:
    InputReaderPolicy();
    virtual ~InputReaderPolicy() { }

    virtual void getReaderConfiguration(InputReaderConfiguration* outConfig);
    virtual sp<PointerControllerInterface> obtainPointerController(int32_t deviceId);
    virtual void notifyInputDevicesChanged(const Vector<InputDeviceInfo>& inputDevices);
    virtual sp<KeyCharacterMap> getKeyboardLayoutOverlay(const String8& inputDeviceDescriptor);
    virtual String8 getDeviceAlias(const InputDeviceIdentifier& identifier);
private:
    sp<android::PointerController> mPointerController;
};

InputReaderPolicy::InputReaderPolicy()
{
    mPointerController = new android::PointerController;
    mPointerController->setDisplaySize(1280, 1024);
    mPointerController->setDisplayOrientation(android::DISPLAY_ORIENTATION_0);
}

void InputReaderPolicy::getReaderConfiguration(InputReaderConfiguration* outConfig)
{
  outConfig->setDisplayInfo(0, /* id */
                            false, /* external  */
                            1280, /* width */
                            1024, /*height*/
                            DISPLAY_ORIENTATION_0 /* orientation */);
}

sp<PointerControllerInterface> InputReaderPolicy::obtainPointerController(int32_t deviceId)
{
    return mPointerController;
}

void InputReaderPolicy::notifyInputDevicesChanged(const Vector<InputDeviceInfo>& inputDevices)
{
}

sp<KeyCharacterMap> InputReaderPolicy::getKeyboardLayoutOverlay(const String8& inputDeviceDescriptor)
{
    return KeyCharacterMap::empty();
}

String8 InputReaderPolicy::getDeviceAlias(const InputDeviceIdentifier& identifier)
{
    return String8();
}

/******************************************************************************
 * main
 ******************************************************************************/

int main()
{
    sp<EventHub> eventHub = new EventHub;
    sp<InputReaderPolicy> inputReaderPolicy = new InputReaderPolicy;
    sp<InputListener> inputListener = new InputListener;

    InputReader* inputReader = new InputReader(eventHub,
                                               inputReaderPolicy,
                                               inputListener);

    while (true) {
        inputReader->loopOnce();
    }

    return 0;
}
