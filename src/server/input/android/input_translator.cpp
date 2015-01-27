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

#define MIR_INCLUDE_DEPRECATED_EVENT_HEADER 

#include "input_translator.h"

#include "androidfw/Input.h"

#include <unordered_set>

namespace mia = mir::input::android;

namespace
{
bool valid_key_event(MirKeyEvent const& key)
{
    return key.action == mir_key_action_up || key.action == mir_key_action_down;
}
inline int32_t get_index_from_motion_action(int action)
{
    // FIXME: https://bugs.launchpad.net/mir/+bug/1311699
    return int32_t(action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
        >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
}

bool valid_motion_event(MirMotionEvent const& motion)
{
    if (motion.pointer_count > MIR_INPUT_EVENT_MAX_POINTER_COUNT)
        return false;

    // FIXME: https://bugs.launchpad.net/mir/+bug/1311699
    switch (motion.action & AMOTION_EVENT_ACTION_MASK)
    {
    case mir_motion_action_down:
    case mir_motion_action_up:
    case mir_motion_action_cancel:
    case mir_motion_action_move:
    case mir_motion_action_outside:
    case mir_motion_action_hover_enter:
    case mir_motion_action_hover_move:
    case mir_motion_action_hover_exit:
    case mir_motion_action_scroll:
        break;
    case mir_motion_action_pointer_down:
    case mir_motion_action_pointer_up:
        {
            int32_t index = get_index_from_motion_action(motion.action);
            if (index < 0 || size_t(index) >= motion.pointer_count)
                return false;
            break;
        }
    default:
        return false;
    }

    std::unordered_set<int> ids;
    for (size_t i = 0; i < motion.pointer_count; ++i)
    {
        int32_t id = motion.pointer_coordinates[i].id;
        if (id < 0 || id > MAX_POINTER_ID)
            return false;
        if (ids.find(id)!=ids.end())
            return false;
        ids.insert(id);
    }

    return true;
}

}

mia::InputTranslator::InputTranslator(std::shared_ptr<InputDispatcher> const& dispatcher)
    : dispatcher(dispatcher)
{
}

void mia::InputTranslator::notifyConfigurationChanged(const droidinput::NotifyConfigurationChangedArgs* args)
{
    dispatcher->configuration_changed(args->eventTime);
}

void mia::InputTranslator::notifyKey(const droidinput::NotifyKeyArgs* args)
{
    if (!args)
        return;
    uint32_t policy_flags = args->policyFlags;
    int32_t modifiers = args->metaState;
    int32_t flags = args->flags;

    if ((policy_flags & droidinput::POLICY_FLAG_VIRTUAL) || (flags & AKEY_EVENT_FLAG_VIRTUAL_HARD_KEY))
        flags |= mir_key_flag_virtual_hard_key;
    if (policy_flags & droidinput::POLICY_FLAG_ALT)
        modifiers |= mir_key_modifier_alt | mir_key_modifier_alt_left;
    if (policy_flags & droidinput::POLICY_FLAG_ALT_GR)
        modifiers |= mir_key_modifier_alt | mir_key_modifier_alt_right;
    if (policy_flags & droidinput::POLICY_FLAG_SHIFT)
        modifiers |= mir_key_modifier_shift | mir_key_modifier_shift_left;
    if (policy_flags & droidinput::POLICY_FLAG_CAPS_LOCK)
        modifiers |= mir_key_modifier_caps_lock;
    if (policy_flags & droidinput::POLICY_FLAG_FUNCTION)
        modifiers |= mir_key_modifier_function;
    if (policy_flags & droidinput::POLICY_FLAG_WOKE_HERE)
        flags |= mir_key_flag_woke_here;

    MirEvent mir_event;
    mir_event.type = mir_event_type_key;
    mir_event.key.device_id = args->deviceId;
    mir_event.key.source_id = args->source;
    mir_event.key.action = static_cast<MirKeyAction>(args->action);
    mir_event.key.flags = static_cast<MirKeyFlag>(flags);
    mir_event.key.modifiers = modifiers;
    mir_event.key.key_code = args->keyCode;
    mir_event.key.scan_code = args->scanCode;
    mir_event.key.repeat_count = 0;
    mir_event.key.down_time = args->downTime;
    mir_event.key.event_time = args->eventTime;
    mir_event.key.is_system_key = false; // TODO: Figure out what this is. //kev->isSystemKey();


    if (!valid_key_event(mir_event.key))
        return;

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

    if (!valid_motion_event(mir_event.motion))
        return;

    dispatcher->dispatch(mir_event);
}

void mia::InputTranslator::notifySwitch(const droidinput::NotifySwitchArgs* /*args*/)
{
    // TODO cannot be expressed through MirEvent
}

void mia::InputTranslator::notifyDeviceReset(const droidinput::NotifyDeviceResetArgs* args)
{
    dispatcher->device_reset(args->deviceId, args->eventTime);
    // TODO cannot be expressed through MirEvent
}

