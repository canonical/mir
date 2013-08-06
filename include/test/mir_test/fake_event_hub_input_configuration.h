/*
 * Copyright © 2013 Canonical Ltd.
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
 * Author: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_FAKE_EVENT_HUB_INPUT_CONFIGURATION_H_
#define MIR_TEST_DOUBLES_FAKE_EVENT_HUB_INPUT_CONFIGURATION_H_

#include "src/server/input/android/default_android_input_configuration.h"

#include <utils/StrongPointer.h>

namespace droidinput = android;

namespace android
{
class EventHubInterface;
}

namespace mir
{
namespace input
{
class CursorListener;
class EventFilter;
class InputReport;

namespace android
{
class FakeEventHub;
}
}
namespace test
{
namespace doubles
{

class FakeEventHubInputConfiguration : public input::android::DefaultInputConfiguration
{
public:
    FakeEventHubInputConfiguration(std::shared_ptr<input::EventFilter> const& event_filter,
                                   std::shared_ptr<input::InputRegion> const& input_region,
                                   std::shared_ptr<input::CursorListener> const& cursor_listener,
                                   std::shared_ptr<input::InputReport> const& input_report);
    virtual ~FakeEventHubInputConfiguration();

    droidinput::sp<droidinput::EventHubInterface> the_event_hub();
    input::android::FakeEventHub* the_fake_event_hub();
    
    bool is_key_repeat_enabled() override { return false; }


protected:
    FakeEventHubInputConfiguration(FakeEventHubInputConfiguration const&) = delete;
    FakeEventHubInputConfiguration& operator=(FakeEventHubInputConfiguration const&) = delete;

private:
    droidinput::sp<input::android::FakeEventHub> event_hub;
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_FAKE_EVENT_HUB_INPUT_CONFIGURATION_H_ */
