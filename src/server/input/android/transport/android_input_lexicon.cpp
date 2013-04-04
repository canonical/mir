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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 *              Robert Carr <robert.carr@canonical.com>
 */

#include "mir/input/android/android_input_lexicon.h"

#include <androidfw/Input.h>

namespace miat = mir::input::android::transport;

void miat::Lexicon::translate(const droidinput::InputEvent *android_event, MirEvent &mir_event)
{
    switch(android_event->getType())
    {
        case AINPUT_EVENT_TYPE_KEY:
        {
            const droidinput::KeyEvent* kev = static_cast<const droidinput::KeyEvent*>(android_event);
            mir_event.type = MIR_INPUT_EVENT_TYPE_KEY;
            mir_event.device_id = android_event->getDeviceId();
            mir_event.source_id = android_event->getSource();
            mir_event.action = kev->getAction();
            mir_event.flags = kev->getFlags();
            mir_event.meta_state = kev->getMetaState();
            mir_event.details.key.key_code = kev->getKeyCode();
            mir_event.details.key.scan_code = kev->getScanCode();
            mir_event.details.key.repeat_count = kev->getRepeatCount();
            mir_event.details.key.down_time = kev->getDownTime();
            mir_event.details.key.event_time = kev->getEventTime();
            mir_event.details.key.is_system_key = false; // TODO: Figure out what this is. //kev->isSystemKey();
            break;
        }
        case AINPUT_EVENT_TYPE_MOTION:
        {
            const droidinput::MotionEvent* mev = static_cast<const droidinput::MotionEvent*>(android_event);
            mir_event.type = MIR_INPUT_EVENT_TYPE_MOTION;
            mir_event.device_id = android_event->getDeviceId();
            mir_event.source_id = android_event->getSource();
            mir_event.action = mev->getAction();
            mir_event.flags = mev->getFlags();
            mir_event.meta_state = mev->getMetaState();
            mir_event.details.motion.edge_flags = mev->getEdgeFlags();
            mir_event.details.motion.button_state = mev->getButtonState();
            mir_event.details.motion.x_offset = mev->getXOffset();
            mir_event.details.motion.y_offset = mev->getYOffset();
            mir_event.details.motion.x_precision = mev->getXPrecision();
            mir_event.details.motion.y_precision = mev->getYPrecision();
            mir_event.details.motion.down_time = mev->getDownTime();
            mir_event.details.motion.event_time = mev->getEventTime();
            mir_event.details.motion.pointer_count = mev->getPointerCount();
            for(unsigned int i = 0; i < mev->getPointerCount(); i++)
            {
                    mir_event.details.motion.pointer_coordinates[i].id = mev->getPointerId(i);
                    mir_event.details.motion.pointer_coordinates[i].x = mev->getX(i);
                    mir_event.details.motion.pointer_coordinates[i].raw_x = mev->getRawX(i);
                    mir_event.details.motion.pointer_coordinates[i].y = mev->getY(i);
                    mir_event.details.motion.pointer_coordinates[i].raw_y = mev->getRawY(i);
                    mir_event.details.motion.pointer_coordinates[i].touch_major = mev->getTouchMajor(i);
                    mir_event.details.motion.pointer_coordinates[i].touch_minor = mev->getTouchMinor(i);
                    mir_event.details.motion.pointer_coordinates[i].size = mev->getSize(i);
                    mir_event.details.motion.pointer_coordinates[i].pressure = mev->getPressure(i);
                    mir_event.details.motion.pointer_coordinates[i].orientation = mev->getOrientation(i);
            }
            break;
        }
    }

}

