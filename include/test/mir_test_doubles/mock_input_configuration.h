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

#ifndef MIR_TEST_DOUBLES_MOCK_INPUT_CONFIGURATION_H_
#define MIR_TEST_DOUBLES_MOCK_INPUT_CONFIGURATION_H_

#include "mir/input/android/android_input_configuration.h"

#include <EventHub.h>

#include <gmock/gmock.h>

namespace droidinput = android;

namespace mir
{
namespace test
{
namespace doubles
{

struct MockInputConfiguration : public input::android::InputConfiguration
{
    MOCK_METHOD0(the_event_hub, droidinput::sp<droidinput::EventHubInterface>());
    MOCK_METHOD0(the_dispatcher, droidinput::sp<droidinput::InputDispatcherInterface>());
    MOCK_METHOD0(the_dispatcher_thread, std::shared_ptr<input::android::InputThread>());
    MOCK_METHOD0(the_reader_thread, std::shared_ptr<input::android::InputThread>());
};

}
}
} // namespace mir

#endif // MIR_TEST_DOUBLES_MOCK_INPUT_CONFIGURATION_H_
