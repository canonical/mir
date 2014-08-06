/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */
#ifndef MIR_ANDROID_INPUT_READER_POLICY_H_
#define MIR_ANDROID_INPUT_READER_POLICY_H_

#include "rudimentary_input_reader_policy.h"

#include <memory>

namespace mir
{
namespace input
{
class CursorListener;
class InputRegion;
class TouchVisualizer;

namespace android
{

class InputReaderPolicy : public RudimentaryInputReaderPolicy
{
public:
    explicit InputReaderPolicy(std::shared_ptr<InputRegion> const& input_region,
                               std::shared_ptr<CursorListener> const& cursor_listener,
                               std::shared_ptr<TouchVisualizer> const& touch_visualizer);

    virtual ~InputReaderPolicy() {}

    droidinput::sp<droidinput::PointerControllerInterface> obtainPointerController(int32_t device_id) override;
    void getReaderConfiguration(droidinput::InputReaderConfiguration* out_config) override;

    void getAssociatedDisplayInfo(droidinput::InputDeviceIdentifier const& identifier,
        int& out_associated_display_id, bool& out_associated_display_is_external) override;
private:
    std::shared_ptr<InputRegion> const input_region;
    droidinput::sp<droidinput::PointerControllerInterface> pointer_controller;
};

}
}
} // namespace mir

#endif // MIR_ANDROID_INPUT_READER_POLICY_H_
