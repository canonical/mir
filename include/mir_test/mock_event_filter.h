/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 */

#ifndef MIR_TEST_MOCK_EVENT_FILTER_H_
#define MIR_TEST_MOCK_EVENT_FILTER_H_

#include "mir/input/event_filter.h"

#include <gmock/gmock.h>

namespace mir
{
struct MockEventFilter : public mir::input::EventFilter
{
    MOCK_METHOD1(handles, bool(const droidinput::InputEvent*));
};
}

MATCHER_P(IsKeyEventWithKey, key, "")
{
    if (arg->getType() != AINPUT_EVENT_TYPE_KEY)
        return false;

    auto key_event = static_cast<const droidinput::KeyEvent*>(arg);
    return key_event->getKeyCode() == key;
}
MATCHER(KeyDownEvent, "")
{
    if (arg->getType() != AINPUT_EVENT_TYPE_KEY)
        return false;

    auto key_event = static_cast<const droidinput::KeyEvent*>(arg);
    return key_event->getAction() == AKEY_EVENT_ACTION_DOWN;
}
MATCHER(ButtonDownEvent, "")
{
    if (arg->getType() != AINPUT_EVENT_TYPE_MOTION)
	return false;
    auto motion_event = static_cast<const droidinput::MotionEvent*>(arg);
    if (motion_event->getButtonState() == 0)
	return false;
    return motion_event->getAction() == AKEY_EVENT_ACTION_DOWN;
}

#endif // MIR_TEST_MOCK_EVENT_FILTER_H_
