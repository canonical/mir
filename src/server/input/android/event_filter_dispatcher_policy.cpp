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

#include "event_filter_dispatcher_policy.h"
#include "mir/input/android/android_input_lexicon.h"
#include "mir/events/event_private.h"

namespace mi = mir::input;
namespace mia = mi::android;

mia::EventFilterDispatcherPolicy::EventFilterDispatcherPolicy(std::shared_ptr<mi::EventFilter> const& event_filter, bool key_repeat_enabled) :
  event_filter(event_filter),
  key_repeat_enabled(key_repeat_enabled)
{
}

void mia::EventFilterDispatcherPolicy::notifyConfigurationChanged(std::chrono::nanoseconds /* when */)
{
}

std::chrono::nanoseconds mia::EventFilterDispatcherPolicy::notifyANR(droidinput::sp<droidinput::InputApplicationHandle> const& /* inputApplicationHandle */,
                                                    droidinput::sp<droidinput::InputWindowHandle> const& /* inputWindowHandle */)
{
    return std::chrono::nanoseconds(0);
}

void mia::EventFilterDispatcherPolicy::notifyInputChannelBroken(droidinput::sp<droidinput::InputWindowHandle> const& /* inputWindowHandle */)
{
}

void mia::EventFilterDispatcherPolicy::getDispatcherConfiguration(droidinput::InputDispatcherConfiguration* /* outConfig */)
{
}

bool mia::EventFilterDispatcherPolicy::isKeyRepeatEnabled()
{
    return key_repeat_enabled;
}

bool mia::EventFilterDispatcherPolicy::filterInputEvent(const droidinput::InputEvent* input_event, uint32_t /*policy_flags*/)
{
    auto mir_ev = mia::Lexicon::translate(input_event);

    // TODO: Use XKBMapper

    return !event_filter->handle(*mir_ev);
}

void mia::EventFilterDispatcherPolicy::interceptKeyBeforeQueueing(const droidinput::KeyEvent* /*key_event*/, uint32_t& policy_flags)
{
    policy_flags |= droidinput::POLICY_FLAG_PASS_TO_USER;
}

void mia::EventFilterDispatcherPolicy::interceptMotionBeforeQueueing(std::chrono::nanoseconds /* when */, uint32_t& policy_flags)
{
    policy_flags |= droidinput::POLICY_FLAG_PASS_TO_USER;
}

std::chrono::nanoseconds mia::EventFilterDispatcherPolicy::interceptKeyBeforeDispatching(
    droidinput::sp<droidinput::InputWindowHandle> const& /* inputWindowHandle */, droidinput::KeyEvent const* /* keyEvent */, uint32_t /* policyFlags */)
{
    return std::chrono::nanoseconds(0);
}

bool mia::EventFilterDispatcherPolicy::dispatchUnhandledKey(droidinput::sp<droidinput::InputWindowHandle> const& /* inputWindowHandle */,
                                                            droidinput::KeyEvent const* /* keyEvent */, uint32_t /* policyFlags */,
                                                            droidinput::KeyEvent* /* outFallbackKeyEvent */)
{
    return false;
}

void mia::EventFilterDispatcherPolicy::notifySwitch(std::chrono::nanoseconds /* when */, int32_t /* switchCode */,
                                                    int32_t /* switchValue */, uint32_t /* policyFlags */)
{
}

void mia::EventFilterDispatcherPolicy::pokeUserActivity(std::chrono::nanoseconds /* eventTime */, int32_t /* eventType */)
{
}

bool mia::EventFilterDispatcherPolicy::checkInjectEventsPermissionNonReentrant(int32_t /* injectorPid */, int32_t /* injectorUid */)
{
    return true;
}
