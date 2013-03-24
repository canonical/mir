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
#include "event_filter_dispatcher_policy.h"
#include "android_input_lexicon.h"

namespace mi = mir::input;
namespace mia = mi::android;

mia::EventFilterDispatcherPolicy::EventFilterDispatcherPolicy(std::shared_ptr<mi::EventFilter> const& event_filter) :
  event_filter(event_filter)
{
}

bool mia::EventFilterDispatcherPolicy::filterInputEvent(const droidinput::InputEvent* input_event, uint32_t /*policy_flags*/)
{
    MirEvent mir_ev;
    mia::Lexicon::translate(input_event, mir_ev);

    if (event_filter->handles(mir_ev))
        return false; /* Do not pass the event on */
    else
        return true; /* Pass the event on */
}

void mia::EventFilterDispatcherPolicy::interceptKeyBeforeQueueing(const droidinput::KeyEvent* /*key_event*/, uint32_t& policy_flags)
{
    policy_flags |= droidinput::POLICY_FLAG_PASS_TO_USER;
}
