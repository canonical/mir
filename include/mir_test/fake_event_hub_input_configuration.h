/*
 * Copyright © 2013 Canonical Ltd.
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
 * Author: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_FAKE_EVENT_HUB_INPUT_CONFIGURATION_H_
#define MIR_TEST_DOUBLES_FAKE_EVENT_HUB_INPUT_CONFIGURATION_H_

#include "src/input/android/default_android_input_configuration.h"

#include <utils/StrongPointer.h>

#include <initializer_list>

namespace droidinput = android;

namespace android
{
class EventHubInterface;
}

namespace mir
{
namespace graphics
{
class ViewableArea;
}
namespace input
{
class CursorListener;
class EventFilter;

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
    FakeEventHubInputConfiguration(std::initializer_list<std::shared_ptr<mir::input::EventFilter> const> const& filters,
                                   std::shared_ptr<mir::graphics::ViewableArea> const& view_area,
                                   std::shared_ptr<mir::input::CursorListener> const& cursor_listener);
    virtual ~FakeEventHubInputConfiguration();

    droidinput::sp<droidinput::EventHubInterface> the_event_hub();
    input::android::FakeEventHub* the_fake_event_hub();
    

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
