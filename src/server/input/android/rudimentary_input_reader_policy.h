/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */
#ifndef MIR_INPUT_ANDROID_RUDIMENTARY_INPUT_READER_POLICY_H_
#define MIR_INPUT_ANDROID_RUDIMENTARY_INPUT_READER_POLICY_H_

// from android
#include <InputReader.h>

namespace droidinput = android;

namespace mir
{
namespace input
{
namespace android
{
class RudimentaryInputReaderPolicy : public droidinput::InputReaderPolicyInterface
{
  public:
    // From InputReaderPolicyInterface
    virtual void getReaderConfiguration(
        droidinput::InputReaderConfiguration* out_config);

    // TODO: We should think about refactoring the device identification approach
    virtual droidinput::sp<droidinput::PointerControllerInterface> obtainPointerController(
        int32_t device_id);

    virtual void notifyInputDevicesChanged(
        const droidinput::Vector<droidinput::InputDeviceInfo>& input_devices);

    virtual droidinput::sp<droidinput::KeyCharacterMap> getKeyboardLayoutOverlay(
        const droidinput::String8& input_device_descriptor);

    virtual droidinput::String8 getDeviceAlias(
        const droidinput::InputDeviceIdentifier& identifier);

  private:
    droidinput::sp<droidinput::PointerController> pointer_controller;
};

}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_RUDIMENTARY_INPUT_READER_POLICY_H_
