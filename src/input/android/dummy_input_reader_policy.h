#ifndef MIR_DUMMY_INPUT_READER_POLICY_H_
#define MIR_DUMMY_INPUT_READER_POLICY_H_

// from android
#include <InputReader.h>

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
class DummyInputReaderPolicy : public droidinput::InputReaderPolicyInterface
{
public:
    DummyInputReaderPolicy()
    {
    }

    virtual void getReaderConfiguration(droidinput::InputReaderConfiguration* out_config)
    {
	// TODO: This needs to go.
        out_config->setDisplayInfo(0, /* id */
                                   false, /* external  */
                                   1280, /* width */
                                   1024, /*height*/
                                   droidinput::DISPLAY_ORIENTATION_0 /* orientation */);
    }

    virtual droidinput::sp<droidinput::PointerControllerInterface> obtainPointerController(int32_t device_id)
    {
        (void)device_id;
        return pointer_controller;
    }

    virtual void notifyInputDevicesChanged(
            const droidinput::Vector<droidinput::InputDeviceInfo>& input_devices)
    {
        (void)input_devices;
    }

    virtual droidinput::sp<droidinput::KeyCharacterMap> getKeyboardLayoutOverlay(
            const droidinput::String8& input_device_descriptor)
    {
        (void)input_device_descriptor;
        return droidinput::KeyCharacterMap::empty();
    }

    virtual droidinput::String8 getDeviceAlias(const droidinput::InputDeviceIdentifier& identifier)
    {
        (void)identifier;
        return droidinput::String8();
    }

    droidinput::sp<droidinput::PointerController> pointer_controller;
};

}
}
} // namespace mir

#endif // MIR_DUMMY_INPUT_READER_POLICY_H_
