/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Robert Carr <robert.carr@canonical.com>
 */

#include "mir/input/android/android_input_lexicon.h"
#include "mir/input/android/event_conversion_helpers.h"
#include "mir/events/event_builders.h"

#include <androidfw/Input.h>

#include <boost/throw_exception.hpp>

#include <vector>
#include <stdexcept>

namespace mia = mir::input::android;
namespace mev = mir::events;

mir::EventUPtr mia::Lexicon::translate(droidinput::InputEvent const* android_event)
{
    switch(android_event->getType())
    {
        case AINPUT_EVENT_TYPE_KEY:
        {
            auto kev = static_cast<const droidinput::KeyEvent*>(android_event);
            return mev::make_event(MirInputDeviceId(android_event->getDeviceId()),
                                   kev->getEventTime(),
                                   mia::mir_keyboard_action_from_android(kev->getAction(), kev->getRepeatCount()),
                                   kev->getKeyCode(),
                                   kev->getScanCode(),
                                   mia::mir_modifiers_from_android(kev->getMetaState()));
        }
        case AINPUT_EVENT_TYPE_MOTION:
        {
            if (mia::android_source_id_is_pointer_device(android_event->getSource()))
            {
                auto mev = static_cast<const droidinput::MotionEvent*>(android_event);
                return mev::make_event(MirInputDeviceId(android_event->getDeviceId()),
                                       mev->getEventTime(),
                                       mia::mir_modifiers_from_android(mev->getMetaState()),
                                       mia::mir_pointer_action_from_masked_android(mev->getAction() & AMOTION_EVENT_ACTION_MASK),
                                       mia::mir_pointer_buttons_from_android(mev->getButtonState()),
                                       mev->getX(0), mev->getY(0),
                                       mev->getRawAxisValue(AMOTION_EVENT_AXIS_HSCROLL, 0),
                                       mev->getRawAxisValue(AMOTION_EVENT_AXIS_VSCROLL, 0),
                                       mev->getRawAxisValue(AMOTION_EVENT_AXIS_RX, 0),
                                       mev->getRawAxisValue(AMOTION_EVENT_AXIS_RY, 0));
            }
            else
            {
                auto mev = static_cast<const droidinput::MotionEvent*>(android_event);
                auto ev = mev::make_event(MirInputDeviceId(android_event->getDeviceId()),
                                          mev->getEventTime(),
                                          mia::mir_modifiers_from_android(mev->getMetaState()));
                auto action = mev->getAction();
                size_t index_with_action = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
                auto masked_action = action & AMOTION_EVENT_ACTION_MASK;
                for (unsigned i = 0; i < mev->getPointerCount(); i++)
                {
                    auto action = (i == index_with_action) ? mia::mir_touch_action_from_masked_android(masked_action) :
                        mir_touch_action_change;
                    mev::add_touch(*ev, mev->getPointerId(i), action, mia::mir_tool_type_from_android(mev->getToolType(i)),
                        mev->getX(i), mev->getY(i),
                        mev->getPressure(i), mev->getTouchMajor(i),
			mev->getTouchMinor(i), mev->getSize(i));
                }
                return ev;
            }
        }
    default:
        BOOST_THROW_EXCEPTION(std::logic_error("Invalid android event"));
    }
}
