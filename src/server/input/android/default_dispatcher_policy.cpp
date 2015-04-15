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

#include "default_dispatcher_policy.h"
#include "mir/input/android/android_input_lexicon.h"
#include "mir/events/event_private.h"

namespace mi = mir::input;
namespace mia = mi::android;

mia::DefaultDispatcherPolicy::DefaultDispatcherPolicy(bool key_repeat_enabled) :
  key_repeat_enabled(key_repeat_enabled)
{
}

void mia::DefaultDispatcherPolicy::notifyConfigurationChanged(std::chrono::nanoseconds /* when */)
{
}

std::chrono::nanoseconds mia::DefaultDispatcherPolicy::notifyANR(droidinput::sp<droidinput::InputApplicationHandle> const& /* inputApplicationHandle */,
                                                    droidinput::sp<droidinput::InputWindowHandle> const& /* inputWindowHandle */)
{
    return std::chrono::nanoseconds(0);
}

void mia::DefaultDispatcherPolicy::notifyInputChannelBroken(droidinput::sp<droidinput::InputWindowHandle> const& /* inputWindowHandle */)
{
}

void mia::DefaultDispatcherPolicy::getDispatcherConfiguration(droidinput::InputDispatcherConfiguration* /* outConfig */)
{
}

bool mia::DefaultDispatcherPolicy::isKeyRepeatEnabled()
{
    return key_repeat_enabled;
}

bool mia::DefaultDispatcherPolicy::filterInputEvent(const droidinput::InputEvent* /* input_event */, uint32_t /*policy_flags*/)
{
    return true;
}

void mia::DefaultDispatcherPolicy::interceptKeyBeforeQueueing(const droidinput::KeyEvent* /*key_event*/, uint32_t& policy_flags)
{
    policy_flags |= droidinput::POLICY_FLAG_PASS_TO_USER;
}

void mia::DefaultDispatcherPolicy::interceptMotionBeforeQueueing(std::chrono::nanoseconds /* when */, uint32_t& policy_flags)
{
    policy_flags |= droidinput::POLICY_FLAG_PASS_TO_USER;
}

std::chrono::nanoseconds mia::DefaultDispatcherPolicy::interceptKeyBeforeDispatching(
    droidinput::sp<droidinput::InputWindowHandle> const& /* inputWindowHandle */, droidinput::KeyEvent const* /* keyEvent */, uint32_t /* policyFlags */)
{
    return std::chrono::nanoseconds(0);
}

bool mia::DefaultDispatcherPolicy::dispatchUnhandledKey(droidinput::sp<droidinput::InputWindowHandle> const& /* inputWindowHandle */,
                                                            droidinput::KeyEvent const* /* keyEvent */, uint32_t /* policyFlags */,
                                                            droidinput::KeyEvent* /* outFallbackKeyEvent */)
{
    return false;
}

void mia::DefaultDispatcherPolicy::notifySwitch(std::chrono::nanoseconds /* when */, int32_t /* switchCode */,
                                                    int32_t /* switchValue */, uint32_t /* policyFlags */)
{
}

void mia::DefaultDispatcherPolicy::pokeUserActivity(std::chrono::nanoseconds /* eventTime */, int32_t /* eventType */)
{
}

bool mia::DefaultDispatcherPolicy::checkInjectEventsPermissionNonReentrant(int32_t /* injectorPid */, int32_t /* injectorUid */)
{
    return true;
}
