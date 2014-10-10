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

#include <androidfw/Input.h>

namespace mia = mir::input::android;

void mia::Lexicon::translate(const droidinput::InputEvent *android_event, MirEvent &mir_event)
{
    switch(android_event->getType())
    {
        case AINPUT_EVENT_TYPE_KEY:
        {
            const droidinput::KeyEvent* kev = static_cast<const droidinput::KeyEvent*>(android_event);
            mir_event.type = mir_event_type_key;
            mir_event.key.device_id = android_event->getDeviceId();
            mir_event.key.source_id = android_event->getSource();
            mir_event.key.action = static_cast<MirKeyAction>(kev->getAction());
            mir_event.key.flags = static_cast<MirKeyFlag>(kev->getFlags());
            mir_event.key.modifiers = kev->getMetaState();
            mir_event.key.key_code = kev->getKeyCode();
            mir_event.key.scan_code = kev->getScanCode();
            mir_event.key.repeat_count = kev->getRepeatCount();
            mir_event.key.down_time = kev->getDownTime();
            mir_event.key.event_time = kev->getEventTime();
            mir_event.key.is_system_key = false; // TODO: Figure out what this is. //kev->isSystemKey();
            break;
        }
        case AINPUT_EVENT_TYPE_MOTION:
        {
            const droidinput::MotionEvent* mev = static_cast<const droidinput::MotionEvent*>(android_event);
            mir_event.type = mir_event_type_motion;
            mir_event.motion.device_id = android_event->getDeviceId();
            mir_event.motion.source_id = android_event->getSource();
            mir_event.motion.action = mev->getAction();
            mir_event.motion.flags = static_cast<MirMotionFlag>(mev->getFlags());
            mir_event.motion.modifiers = mev->getMetaState();
            mir_event.motion.edge_flags = mev->getEdgeFlags();
            mir_event.motion.button_state = static_cast<MirMotionButton>(mev->getButtonState());
            mir_event.motion.x_offset = mev->getXOffset();
            mir_event.motion.y_offset = mev->getYOffset();
            mir_event.motion.x_precision = mev->getXPrecision();
            mir_event.motion.y_precision = mev->getYPrecision();
            mir_event.motion.down_time = mev->getDownTime();
            mir_event.motion.event_time = mev->getEventTime();
            mir_event.motion.pointer_count = mev->getPointerCount();
            for(unsigned int i = 0; i < mev->getPointerCount(); i++)
            {
                    mir_event.motion.pointer_coordinates[i].id = mev->getPointerId(i);
                    mir_event.motion.pointer_coordinates[i].x = mev->getX(i);
                    mir_event.motion.pointer_coordinates[i].raw_x = mev->getRawX(i);
                    mir_event.motion.pointer_coordinates[i].y = mev->getY(i);
                    mir_event.motion.pointer_coordinates[i].raw_y = mev->getRawY(i);
                    mir_event.motion.pointer_coordinates[i].touch_major = mev->getTouchMajor(i);
                    mir_event.motion.pointer_coordinates[i].touch_minor = mev->getTouchMinor(i);
                    mir_event.motion.pointer_coordinates[i].size = mev->getSize(i);
                    mir_event.motion.pointer_coordinates[i].pressure = mev->getPressure(i);
                    mir_event.motion.pointer_coordinates[i].orientation = mev->getOrientation(i);

                    mir_event.motion.pointer_coordinates[i].vscroll =
                           mev->getRawAxisValue(AMOTION_EVENT_AXIS_VSCROLL, i);

                    mir_event.motion.pointer_coordinates[i].hscroll =
                           mev->getRawAxisValue(AMOTION_EVENT_AXIS_HSCROLL, i);

                    mir_event.motion.pointer_coordinates[i].tool_type =
                           static_cast<MirMotionToolType>(mev->getToolType(i));
            }
            break;
        }
    }

}

