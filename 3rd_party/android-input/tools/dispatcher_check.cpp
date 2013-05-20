
#include <InputManager.h>
#include <pthread.h>
#include <std/Thread.h>
#include <iostream>

using namespace android;

/******************************************************************************
 * InputReaderPolicy
 ******************************************************************************/

class InputReaderPolicy : public InputReaderPolicyInterface
{
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
 * InputDispatcherPolicy
 ******************************************************************************/

class InputDispatcherPolicy : public InputDispatcherPolicyInterface
{
public:
    InputDispatcherPolicy() {}
    virtual ~InputDispatcherPolicy() {}

    virtual void notifyConfigurationChanged(nsecs_t when);
    virtual nsecs_t notifyANR(const sp<InputApplicationHandle>& inputApplicationHandle,
            const sp<InputWindowHandle>& inputWindowHandle);
    virtual void notifyInputChannelBroken(const sp<InputWindowHandle>& inputWindowHandle);
    virtual void getDispatcherConfiguration(InputDispatcherConfiguration* outConfig);
    virtual bool isKeyRepeatEnabled();
    virtual bool filterInputEvent(const InputEvent* inputEvent, uint32_t policyFlags);
    virtual void interceptKeyBeforeQueueing(const KeyEvent* keyEvent, uint32_t& policyFlags);
    virtual void interceptMotionBeforeQueueing(nsecs_t when, uint32_t& policyFlags);
    virtual nsecs_t interceptKeyBeforeDispatching(const sp<InputWindowHandle>& inputWindowHandle,
            const KeyEvent* keyEvent, uint32_t policyFlags);
    virtual bool dispatchUnhandledKey(const sp<InputWindowHandle>& inputWindowHandle,
            const KeyEvent* keyEvent, uint32_t policyFlags, KeyEvent* outFallbackKeyEvent);
    virtual void notifySwitch(nsecs_t when,
            int32_t switchCode, int32_t switchValue, uint32_t policyFlags);
    virtual void pokeUserActivity(nsecs_t eventTime, int32_t eventType);
    virtual bool checkInjectEventsPermissionNonReentrant(
            int32_t injectorPid, int32_t injectorUid);
};

void InputDispatcherPolicy::notifyConfigurationChanged(nsecs_t when)
{
    (void)when;
}

nsecs_t InputDispatcherPolicy::notifyANR(
    const sp<InputApplicationHandle>& inputApplicationHandle,
    const sp<InputWindowHandle>& inputWindowHandle)
{
    (void)inputApplicationHandle;
    (void)inputWindowHandle;
    return 0;
}

void InputDispatcherPolicy::notifyInputChannelBroken(
    const sp<InputWindowHandle>& inputWindowHandle)
{
    (void)inputWindowHandle;
}

void InputDispatcherPolicy::getDispatcherConfiguration(InputDispatcherConfiguration* outConfig)
{
   (void)outConfig;
}

bool InputDispatcherPolicy::isKeyRepeatEnabled()
{
    return true;
}

bool InputDispatcherPolicy::filterInputEvent(const InputEvent* inputEvent, uint32_t policyFlags)
{
    (void)inputEvent;
    (void)policyFlags;
    return true;
}

void InputDispatcherPolicy::interceptKeyBeforeQueueing(const KeyEvent* keyEvent,
                                                       uint32_t& policyFlags)
{
    (void)keyEvent;
    policyFlags = POLICY_FLAG_PASS_TO_USER;
}

void InputDispatcherPolicy::interceptMotionBeforeQueueing(nsecs_t when, uint32_t& policyFlags)
{
    (void)when;
    policyFlags = POLICY_FLAG_PASS_TO_USER;
}

nsecs_t InputDispatcherPolicy::interceptKeyBeforeDispatching(
    const sp<InputWindowHandle>& inputWindowHandle,
    const KeyEvent* keyEvent, uint32_t policyFlags)
{
    (void)inputWindowHandle;
    (void)keyEvent;
    (void)policyFlags;
    return 0;
}

bool InputDispatcherPolicy::dispatchUnhandledKey(const sp<InputWindowHandle>& inputWindowHandle,
        const KeyEvent* keyEvent, uint32_t policyFlags, KeyEvent* outFallbackKeyEvent)
{
    (void)inputWindowHandle;
    (void)keyEvent;
    (void)policyFlags;
    (void)outFallbackKeyEvent;
    return false;
}

void InputDispatcherPolicy::notifySwitch(nsecs_t when,
        int32_t switchCode, int32_t switchValue, uint32_t policyFlags)
{
    (void)when;
    (void)switchCode;
    (void)switchValue;
    (void)policyFlags;
}

void InputDispatcherPolicy::pokeUserActivity(nsecs_t eventTime, int32_t eventType)
{
    (void)eventTime;
    (void)eventType;
}

bool InputDispatcherPolicy::checkInjectEventsPermissionNonReentrant(
        int32_t injectorPid, int32_t injectorUid)
{
    (void)injectorPid;
    (void)injectorUid;
    return true;
}

/******************************************************************************
 * FakeWindowHandle
 ******************************************************************************/

class FakeWindowHandle : public InputWindowHandle
{
public:
    FakeWindowHandle(const sp<InputApplicationHandle>& inputApplicationHandle)
        : InputWindowHandle(inputApplicationHandle) {
        mInfo = new InputWindowInfo;
    }
    virtual ~FakeWindowHandle() {}

    bool updateInfo() { return true; }

    InputWindowInfo* info() {return mInfo;}
};

/******************************************************************************
 * FakeApplicationHandle
 ******************************************************************************/

class FakeApplicationHandle : public InputApplicationHandle
{
public:
    virtual bool updateInfo() { return true; }
};

/******************************************************************************
 * ClientThread
 ******************************************************************************/

class ClientThread : public Thread {
public:
    explicit ClientThread(const sp<InputChannel>& inputChannel);
    ~ClientThread() {}

private:
    virtual bool threadLoop();
    void printEvent(InputEvent *event, uint32_t seq);

    InputConsumer inputConsumer;
    PooledInputEventFactory eventFactory;
};

ClientThread::ClientThread(const sp<InputChannel>& inputChannel)
    : Thread(), inputConsumer(inputChannel)
{
}

bool ClientThread::threadLoop()
{
    status_t status;
    uint32_t seq;
    nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
    InputEvent *event;

    status = inputConsumer.consume(&eventFactory, true /* consumeBatches */,
            now, &seq, &event);

    if (status == OK) {
        printEvent(event, seq);
        inputConsumer.sendFinishedSignal(seq, true /* handled */);
    }

    // Continue the thread if this condition is met
    return status == OK || status == WOULD_BLOCK;
}

void ClientThread::printEvent(InputEvent *event, uint32_t seq)
{
    std::cout << "client got event, seq " << seq << std::endl;
}

/******************************************************************************
 * main
 ******************************************************************************/

int main()
{
    sp<EventHub> eventHub = new EventHub;
    sp<InputReaderPolicy> inputReaderPolicy = new InputReaderPolicy;
    sp<InputDispatcherPolicy> inputDispatcherPolicy = new InputDispatcherPolicy;
    sp<InputManager> inputManager = new InputManager(eventHub,
                                                     inputReaderPolicy,
                                                     inputDispatcherPolicy);


    sp<FakeWindowHandle> windowHandle = new FakeWindowHandle(new FakeApplicationHandle);
    Vector<sp<InputWindowHandle> > inputWindowHandles;
    inputWindowHandles.push(windowHandle);

    sp<InputChannel> serverChannel, clientChannel;
    status_t status = InputChannel::openInputChannelPair(String8("foo"),
                                                         serverChannel,
                                                         clientChannel);
    assert(status == OK);

    windowHandle->info()->inputChannel = serverChannel;
    windowHandle->info()->hasFocus = true;

    sp<InputDispatcherInterface> dispatcher = inputManager->getDispatcher();
    dispatcher->setInputWindows(inputWindowHandles);
    dispatcher->registerInputChannel(serverChannel, windowHandle, false);
    dispatcher->setInputDispatchMode(true /* enabled */, false /* frozen */);

    inputManager->start();

    sp<ClientThread> clientThread = new ClientThread(clientChannel);
    clientThread->run();

    clientThread->join();

    inputManager->stop();

    return 0;
}
