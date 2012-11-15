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

#include "android_input_lexicon.h"

#include <androidfw/Input.h>

namespace mia = mir::input::android;

void mia::Lexicon::translate(droidinput::InputEvent *android_event, MirEvent *out_mir_event)
{
    switch(android_event->getType())
    {
        case AINPUT_EVENT_TYPE_KEY:
	{
	    const droidinput::KeyEvent* kev = static_cast<const droidinput::KeyEvent*>(android_event);
	    out_mir_event->type = MIR_INPUT_EVENT_TYPE_KEY;
	    out_mir_event->device_id = android_event->getDeviceId();
	    out_mir_event->source_id = android_event->getSource();
	    out_mir_event->action = kev->getAction();
	    out_mir_event->flags = kev->getFlags();
	    out_mir_event->meta_state = kev->getMetaState();
	    out_mir_event->details.key.key_code = kev->getKeyCode();
	    out_mir_event->details.key.scan_code = kev->getScanCode();
	    out_mir_event->details.key.repeat_count = kev->getRepeatCount();
	    out_mir_event->details.key.down_time = kev->getDownTime();
	    out_mir_event->details.key.event_time = kev->getEventTime();
	    out_mir_event->details.key.is_system_key = false; // TODO: Figure out what this is. //kev->isSystemKey();
	    break;
	}
    case AINPUT_EVENT_TYPE_MOTION:
	break;
	/*
	const android::MotionEvent* mev = static_cast<const android::MotionEvent*>(ev);
	e.type = MOTION_EVENT_TYPE;
	e.device_id = android_event->getDeviceId();
	e.source_id = android_event->getSource();
	e.action = mev->getAction();
	e.flags = mev->getFlags();
	e.meta_state = mev->getMetaState();
	e.details.motion.edge_flags = mev->getEdgeFlags();
	e.details.motion.button_state = mev->getButtonState();
	e.details.motion.x_offset = mev->getXOffset();
	e.details.motion.y_offset = mev->getYOffset();
	e.details.motion.x_precision = mev->getXPrecision();
	e.details.motion.y_precision = mev->getYPrecision();
	e.details.motion.down_time = mev->getDownTime();
	e.details.motion.event_time = mev->getEventTime();
	e.details.motion.pointer_count = mev->getPointerCount();
	for(unsigned int i = 0; i < mev->getPointerCount(); i++)
        {
		e.details.motion.pointer_coordinates[i].id = mev->getPointerId(i);
		e.details.motion.pointer_coordinates[i].x = mev->getX(i);
		e.details.motion.pointer_coordinates[i].raw_x = mev->getRawX(i);
		e.details.motion.pointer_coordinates[i].y = mev->getY(i);
		e.details.motion.pointer_coordinates[i].raw_y = mev->getRawY(i);
		e.details.motion.pointer_coordinates[i].touch_major = mev->getTouchMajor(i);
		e.details.motion.pointer_coordinates[i].touch_minor = mev->getTouchMinor(i);
		e.details.motion.pointer_coordinates[i].size = mev->getSize(i);
		e.details.motion.pointer_coordinates[i].pressure = mev->getPressure(i);
		e.details.motion.pointer_coordinates[i].orientation = mev->getOrientation(i);
        }
	break;*/
    }

}

