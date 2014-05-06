/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "input_translator.h"

namespace mia = mir::input::android;

mia::InputTranslator::InputTranslator(std::shared_ptr<InputDispatcher> const& dispatcher)
    : dispatcher(dispatcher)
{
}

void mia::InputTranslator::notifyConfigurationChanged(const droidinput::NotifyConfigurationChangedArgs* /*args*/)
{
    // TODO cannot be expressed through MirEvent
}

void mia::InputTranslator::notifyKey(const droidinput::NotifyKeyArgs* args)
{
    if (!args)
        return;

    MirEvent mir_event;
    mir_event.type = mir_event_type_key;
    mir_event.key.device_id = args->deviceId;
    mir_event.key.source_id = args->source;
    mir_event.key.action = static_cast<MirKeyAction>(args->action);
    mir_event.key.flags = static_cast<MirKeyFlag>(args->flags);
    mir_event.key.modifiers = args->metaState;
    mir_event.key.key_code = args->keyCode;
    mir_event.key.scan_code = args->scanCode;
    mir_event.key.repeat_count = 0;
    mir_event.key.down_time = args->downTime;
    mir_event.key.event_time = args->eventTime;
    mir_event.key.is_system_key = false; // TODO: Figure out what this is. //kev->isSystemKey();

    dispatcher->dispatch(mir_event);
}

void mia::InputTranslator::notifyMotion(const droidinput::NotifyMotionArgs* args)
{
    if (!args)
        return;

    MirEvent mir_event;
    mir_event.type = mir_event_type_motion;
    mir_event.motion.device_id = args->deviceId;
    mir_event.motion.source_id = args->source;
    mir_event.motion.action = args->action;
    mir_event.motion.flags = static_cast<MirMotionFlag>(args->flags);
    mir_event.motion.modifiers = args->metaState;
    mir_event.motion.edge_flags = args->edgeFlags;
    mir_event.motion.button_state = static_cast<MirMotionButton>(args->buttonState);
    mir_event.motion.x_offset = 0; // offsets or axis positions are calculated in dispatcher
    mir_event.motion.y_offset = 0;
    mir_event.motion.x_precision = args->xPrecision;
    mir_event.motion.y_precision = args->yPrecision;
    mir_event.motion.down_time = args->downTime;
    mir_event.motion.event_time = args->eventTime;
    mir_event.motion.pointer_count = args->pointerCount;
    for(unsigned int i = 0; i < args->pointerCount; i++)
    {
        mir_event.motion.pointer_coordinates[i].id = args->pointerProperties[i].id;
        mir_event.motion.pointer_coordinates[i].x = args->pointerCoords[i].getX();
        mir_event.motion.pointer_coordinates[i].y = args->pointerCoords[i].getY();
        // offsets or axis positions are calculated in dispatcher:
        mir_event.motion.pointer_coordinates[i].raw_x = args->pointerCoords[i].getX();
        mir_event.motion.pointer_coordinates[i].raw_y = args->pointerCoords[i].getY();
        mir_event.motion.pointer_coordinates[i].touch_major =
            args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR);
        mir_event.motion.pointer_coordinates[i].touch_minor =
            args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR);
        mir_event.motion.pointer_coordinates[i].size =
            args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_SIZE);
        mir_event.motion.pointer_coordinates[i].pressure =
            args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_PRESSURE);
        mir_event.motion.pointer_coordinates[i].orientation =
            args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_ORIENTATION);

        mir_event.motion.pointer_coordinates[i].vscroll =
            args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_VSCROLL);

        mir_event.motion.pointer_coordinates[i].hscroll =
            args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_HSCROLL);

        mir_event.motion.pointer_coordinates[i].tool_type =
            static_cast<MirMotionToolType>(args->pointerProperties[i].toolType);
    }

    dispatcher->dispatch(mir_event);
}

void mia::InputTranslator::notifySwitch(const droidinput::NotifySwitchArgs* /*args*/)
{
    // TODO cannot be expressed through MirEvent
}

void mia::InputTranslator::notifyDeviceReset(const droidinput::NotifyDeviceResetArgs* /*args*/)
{
    // TODO cannot be expressed through MirEvent
}

