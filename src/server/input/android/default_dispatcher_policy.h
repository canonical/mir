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

#ifndef MIR_INPUT_ANDROID_EVENT_FILTER_DISPATCHER_POLICY_H_
#define MIR_INPUT_ANDROID_EVENT_FILTER_DISPATCHER_POLICY_H_

#include "mir/input/event_filter.h"

#include <InputDispatcher.h>

namespace android
{
class InputEvent;
}

namespace droidinput = android;

namespace mir
{
namespace input
{
namespace android
{
//class EventFilter;

class DefaultDispatcherPolicy : public droidinput::InputDispatcherPolicyInterface
{
public:
    explicit DefaultDispatcherPolicy();
    virtual ~DefaultDispatcherPolicy() {}

    void notifyConfigurationChanged(std::chrono::nanoseconds when);
    std::chrono::nanoseconds notifyANR(droidinput::sp<droidinput::InputApplicationHandle> const& inputApplicationHandle,
        droidinput::sp<droidinput::InputWindowHandle> const& inputWindowHandle);
    void notifyInputChannelBroken(droidinput::sp<droidinput::InputWindowHandle> const& inputWindowHandle);
    bool filterInputEvent(const droidinput::InputEvent* input_event,
        uint32_t policy_flags);
    void interceptKeyBeforeQueueing(const droidinput::KeyEvent* key_event,
        uint32_t& policy_flags);
    void getDispatcherConfiguration(droidinput::InputDispatcherConfiguration* outConfig);
    bool isKeyRepeatEnabled();
    void interceptMotionBeforeQueueing(std::chrono::nanoseconds when, uint32_t& policyFlags);

    std::chrono::nanoseconds interceptKeyBeforeDispatching(droidinput::sp<droidinput::InputWindowHandle> const& inputWindowHandle,
        droidinput::KeyEvent const* keyEvent, uint32_t policyFlags);

    bool dispatchUnhandledKey(droidinput::sp<droidinput::InputWindowHandle> const& inputWindowHandle,
                              droidinput::KeyEvent const* keyEvent, uint32_t policyFlags,
                              droidinput::KeyEvent* outFallbackKeyEvent);

    void notifySwitch(std::chrono::nanoseconds when, int32_t switchCode, int32_t switchValue, uint32_t policyFlags);
    void pokeUserActivity(std::chrono::nanoseconds eventTime, int32_t eventType);
    bool checkInjectEventsPermissionNonReentrant(int32_t injectorPid, int32_t injectorUid);

protected:
    DefaultDispatcherPolicy(const DefaultDispatcherPolicy&) = delete;
    DefaultDispatcherPolicy& operator=(const DefaultDispatcherPolicy&) = delete;
};

}
}
}

#endif // MIR_INPUT_ANDROID_EVENT_FILTER_DISPATCHER_POLICY_H_
