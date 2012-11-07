#ifndef MIR_DUMMY_INPUT_READER_POLICY_H_
#define MIR_DUMMY_INPUT_READER_POLICY_H_

// from android
#include <InputReader.h>

namespace mir
{
namespace input
{
/*
 Dummy implementation of a InputReaderPolicy

 Delete it once we have a real implementation.
 */
class DummyInputReaderPolicy : public android::InputReaderPolicyInterface
{
public:
    DummyInputReaderPolicy()
    {
        pointer_controller = new android::PointerController;
        pointer_controller->setDisplaySize(1280, 1024);
        pointer_controller->setDisplayOrientation(android::DISPLAY_ORIENTATION_0);
    }

    virtual void getReaderConfiguration(android::InputReaderConfiguration* out_config)
    {
        out_config->setDisplayInfo(0, /* id */
                                   false, /* external  */
                                   1280, /* width */
                                   1024, /*height*/
                                   android::DISPLAY_ORIENTATION_0 /* orientation */);
    }

    virtual android::sp<android::PointerControllerInterface> obtainPointerController(int32_t device_id)
    {
        (void)device_id;
        return pointer_controller;
    }

    virtual void notifyInputDevicesChanged(
            const android::Vector<android::InputDeviceInfo>& input_devices)
    {
        (void)input_devices;
    }

    virtual android::sp<android::KeyCharacterMap> getKeyboardLayoutOverlay(
            const android::String8& input_device_descriptor)
    {
        (void)input_device_descriptor;
        return android::KeyCharacterMap::empty();
    }

    virtual android::String8 getDeviceAlias(const android::InputDeviceIdentifier& identifier)
    {
        (void)identifier;
        return android::String8();
    }

    android::sp<android::PointerController> pointer_controller;
};
}
} // namespace mir

#endif // MIR_DUMMY_INPUT_READER_POLICY_H_
