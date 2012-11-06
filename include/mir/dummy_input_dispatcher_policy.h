#ifndef MIR_DUMMY_INPUT_DISPATCHER_POLICY_H_
#define MIR_DUMMY_INPUT_DISPATCHER_POLICY_H_

// from android
#include <InputDispatcher.h>

namespace android
{
    class InputEvent;
}

namespace mir
{

/*
 Dummy implementation of a InputReaderPolicy

 Delete it once we have a real implementation.
 */
class DummyInputDispatcherPolicy : public android::InputDispatcherPolicyInterface
{
public:
    DummyInputDispatcherPolicy() {}
    virtual ~DummyInputDispatcherPolicy() {}

    virtual void notifyConfigurationChanged(nsecs_t when)
    {
        (void)when;
    }

    virtual nsecs_t notifyANR(
            const android::sp<android::InputApplicationHandle>& inputApplicationHandle,
            const android::sp<android::InputWindowHandle>& inputWindowHandle)
    {
        (void)inputApplicationHandle;
        (void)inputWindowHandle;
        return 0;
    }

    virtual void notifyInputChannelBroken(
            const android::sp<android::InputWindowHandle>& inputWindowHandle)
    {
        (void)inputWindowHandle;
    }

    virtual void getDispatcherConfiguration(
            android::InputDispatcherConfiguration* outConfig)
    {
        (void)outConfig;
    }

    virtual bool isKeyRepeatEnabled()
    {
        return true;
    }

    virtual bool filterInputEvent(const android::InputEvent* inputEvent,
                                  uint32_t policyFlags)
    {
        (void)inputEvent;
        (void)policyFlags;
        return true;
    }

    virtual void interceptKeyBeforeQueueing(const android::KeyEvent* keyEvent,
                                            uint32_t& policyFlags)
    {
        (void)keyEvent;
        policyFlags = android::POLICY_FLAG_PASS_TO_USER;
    }

    virtual void interceptMotionBeforeQueueing(nsecs_t when, uint32_t& policyFlags)
    {
        (void)when;
        policyFlags = android::POLICY_FLAG_PASS_TO_USER;
    }

    virtual nsecs_t interceptKeyBeforeDispatching(
            const android::sp<android::InputWindowHandle>& inputWindowHandle,
            const android::KeyEvent* keyEvent, uint32_t policyFlags)
    {
        (void)inputWindowHandle;
        (void)keyEvent;
        (void)policyFlags;
        return 0;
    }

    virtual bool dispatchUnhandledKey(
            const android::sp<android::InputWindowHandle>& inputWindowHandle,
            const android::KeyEvent* keyEvent, uint32_t policyFlags,
            android::KeyEvent* outFallbackKeyEvent)
    {
        (void)inputWindowHandle;
        (void)keyEvent;
        (void)policyFlags;
        (void)outFallbackKeyEvent;
        return false;
    }

    virtual void notifySwitch(nsecs_t when,
            int32_t switchCode, int32_t switchValue, uint32_t policyFlags)
    {
        (void)when;
        (void)switchCode;
        (void)switchValue;
        (void)policyFlags;
    }

    virtual void pokeUserActivity(nsecs_t eventTime, int32_t eventType)
    {
        (void)eventTime;
        (void)eventType;
    }

    virtual bool checkInjectEventsPermissionNonReentrant(
            int32_t injectorPid, int32_t injectorUid)
    {
        (void)injectorPid;
        (void)injectorUid;
        return true;
    }
};

}

#endif // MIR_DUMMY_INPUT_DISPATCHER_POLICY_H_
