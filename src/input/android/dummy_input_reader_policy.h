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
        static const int32_t default_display_id = 0;
        static const bool is_external = false;
        static const int32_t display_width = 1024;
        static const int32_t display_height = 1024;
        static const int32_t display_orientation = droidinput::DISPLAY_ORIENTATION_0;
        // TODO: This needs to go.
        out_config->setDisplayInfo(
            default_display_id,
            is_external,
            display_width,
            display_height,
            display_orientation);
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
