#ifndef MIR_DUMMY_INPUT_DISPATCHER_POLICY_H_
#define MIR_DUMMY_INPUT_DISPATCHER_POLICY_H_

// from android
#include <InputDispatcher.h>

namespace android
{
class InputEvent;
}

namespace droidinput = android;

namespace mir
{
namespace input
{
namespace android
{
/*
 Dummy implementation of a InputReaderPolicy

 Delete it once we have a real implementation.
 */
class DummyInputDispatcherPolicy : public droidinput::InputDispatcherPolicyInterface
{
public:
    DummyInputDispatcherPolicy() {}
    virtual ~DummyInputDispatcherPolicy() {}

    virtual void notifyConfigurationChanged(nsecs_t when)
    {
        (void)when;
    }

    virtual nsecs_t notifyANR(
            const droidinput::sp<droidinput::InputApplicationHandle>& inputApplicationHandle,
            const droidinput::sp<droidinput::InputWindowHandle>& inputWindowHandle)
    {
        (void)inputApplicationHandle;
        (void)inputWindowHandle;
        return 0;
    }

    virtual void notifyInputChannelBroken(
            const droidinput::sp<droidinput::InputWindowHandle>& inputWindowHandle)
    {
        (void)inputWindowHandle;
    }

    virtual void getDispatcherConfiguration(
            droidinput::InputDispatcherConfiguration* outConfig)
    {
        (void)outConfig;
    }

    virtual bool isKeyRepeatEnabled()
    {
        return true;
    }

    virtual bool filterInputEvent(const droidinput::InputEvent* inputEvent,
                                  uint32_t policyFlags)
    {
        (void)inputEvent;
        (void)policyFlags;
        return true;
    }

    virtual void interceptKeyBeforeQueueing(const droidinput::KeyEvent* keyEvent,
                                            uint32_t& policyFlags)
    {
        (void)keyEvent;
        policyFlags = droidinput::POLICY_FLAG_PASS_TO_USER;
    }

    virtual void interceptMotionBeforeQueueing(nsecs_t when, uint32_t& policyFlags)
    {
        (void)when;
        policyFlags = droidinput::POLICY_FLAG_PASS_TO_USER;
    }

    virtual nsecs_t interceptKeyBeforeDispatching(
            const droidinput::sp<droidinput::InputWindowHandle>& inputWindowHandle,
            const droidinput::KeyEvent* keyEvent, uint32_t policyFlags)
    {
        (void)inputWindowHandle;
        (void)keyEvent;
        (void)policyFlags;
        return 0;
    }

    virtual bool dispatchUnhandledKey(
            const droidinput::sp<droidinput::InputWindowHandle>& inputWindowHandle,
            const droidinput::KeyEvent* keyEvent, uint32_t policyFlags,
            droidinput::KeyEvent* outFallbackKeyEvent)
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
}
}

#endif // MIR_DUMMY_INPUT_DISPATCHER_POLICY_H_
