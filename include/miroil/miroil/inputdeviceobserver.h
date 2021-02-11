#ifndef MIROIL_INPUT_DEVICE_OBSERVER_H
#define MIROIL_INPUT_DEVICE_OBSERVER_H
#include <mir_toolkit/mir_input_device_types.h>
#include <memory>
#include <string>

namespace mir { namespace input { class Device; } }

namespace miroil
{
class InputDeviceObserver
{
public:
    virtual ~InputDeviceObserver();
    
    void             applyKeymap(const std::shared_ptr<mir::input::Device> &device, const std::string & layout, const std::string & variant);
    MirInputDeviceId getDeviceID(const std::shared_ptr<mir::input::Device> &device);
    std::string      getDeviceName(const std::shared_ptr<mir::input::Device> &device);
    bool             isKeyboard(const std::shared_ptr<mir::input::Device> &device);
    bool             isAlphaNumeric(const std::shared_ptr<mir::input::Device> &device);
    
    virtual void device_added(const std::shared_ptr<mir::input::Device> &device) = 0;
    virtual void device_removed(const std::shared_ptr<mir::input::Device> &device) = 0;
};
    
}

#endif //MIROIL_INPUT_DEVICE_OBSERVER_H


