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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_FAKE_EVENT_HUB_INPUT_CONFIGURATION_H_
#define MIR_TEST_DOUBLES_FAKE_EVENT_HUB_INPUT_CONFIGURATION_H_

#include "mir/input/android/default_android_input_configuration.h"

#include <memory>

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
class TouchVisualizer;
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
    FakeEventHubInputConfiguration(
        std::shared_ptr<input::InputDispatcher> const& input_dispatcher,
        std::shared_ptr<input::InputRegion> const& input_region,
        std::shared_ptr<input::CursorListener> const& cursor_listener,
        std::shared_ptr<input::TouchVisualizer> const& touch_visualizer,
        std::shared_ptr<input::InputReport> const& input_report);

    virtual ~FakeEventHubInputConfiguration();

    std::shared_ptr<droidinput::EventHubInterface> the_event_hub();
    input::android::FakeEventHub* the_fake_event_hub();

protected:
    FakeEventHubInputConfiguration(FakeEventHubInputConfiguration const&) = delete;
    FakeEventHubInputConfiguration& operator=(FakeEventHubInputConfiguration const&) = delete;

private:
    std::shared_ptr<input::android::FakeEventHub> event_hub;
};

}
}
} // namespace mir

#endif /* MIR_TEST_DOUBLES_FAKE_EVENT_HUB_INPUT_CONFIGURATION_H_ */
