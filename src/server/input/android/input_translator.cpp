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

#include "mir/input/android/event_conversion_helpers.h"
#include "mir/events/event_builders.h"
#include "mir/events/event_private.h"

#include "androidfw/Input.h"

#include <unordered_set>

namespace mia = mir::input::android;
namespace mev = mir::events;

namespace
{

bool valid_motion_event(MirMotionEvent const& motion)
{
    if (motion.pointer_count > MIR_INPUT_EVENT_MAX_POINTER_COUNT)
        return false;

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
    MirEvent mir_event;
    mir_event.type = mir_event_type_input_configuration;
    auto& idev = mir_event.input_configuration;
    idev.action = mir_input_configuration_action_configuration_changed;
    idev.when = args->eventTime;
    idev.id = -1;

    dispatcher->dispatch(mir_event);
}

void mia::InputTranslator::notifyKey(const droidinput::NotifyKeyArgs* args)
{
    if (!args)
        return;
    uint32_t policy_flags = args->policyFlags;
    MirInputEventModifiers mir_modifiers = mia::mir_modifiers_from_android(args->metaState);

    if (policy_flags & droidinput::POLICY_FLAG_ALT)
        mir_modifiers |= mir_input_event_modifier_alt | mir_input_event_modifier_alt_left;
    if (policy_flags & droidinput::POLICY_FLAG_ALT_GR)
        mir_modifiers |= mir_input_event_modifier_alt | mir_input_event_modifier_alt_right;
    if (policy_flags & droidinput::POLICY_FLAG_SHIFT)
        mir_modifiers |= mir_input_event_modifier_shift | mir_input_event_modifier_shift_left;
    if (policy_flags & droidinput::POLICY_FLAG_CAPS_LOCK)
        mir_modifiers |= mir_input_event_modifier_caps_lock;
    if (policy_flags & droidinput::POLICY_FLAG_FUNCTION)
        mir_modifiers |= mir_input_event_modifier_function;

    // If we've added a modifier to none we have to remove the none flag.
    if (mir_modifiers != mir_input_event_modifier_none && mir_modifiers & mir_input_event_modifier_none)
    {
        mir_modifiers &= ~mir_input_event_modifier_none;
    }

    auto mir_event = mev::make_event(
        MirInputDeviceId(args->deviceId),
        args->eventTime,
        mia::mir_keyboard_action_from_android(args->action, 0 /* repeat_count */),
        args->keyCode,
        args->scanCode,
        mir_modifiers);

    dispatcher->dispatch(*mir_event);
}

void mia::InputTranslator::notifyMotion(const droidinput::NotifyMotionArgs* args)
{
    if (!args)
        return;

    if (mia::android_source_id_is_pointer_device(args->source))
    {
        auto const& pc = args->pointerCoords[0];
        auto mir_event = mev::make_event(MirInputDeviceId(args->deviceId),
            args->eventTime,
            mia::mir_modifiers_from_android(args->metaState),
            mia::mir_pointer_action_from_masked_android(args->action & AMOTION_EVENT_ACTION_MASK),
            mia::mir_pointer_buttons_from_android(args->buttonState),
            pc.getX(), pc.getY(),
            pc.getAxisValue(AMOTION_EVENT_AXIS_HSCROLL),
            pc.getAxisValue(AMOTION_EVENT_AXIS_VSCROLL),
            pc.getAxisValue(AMOTION_EVENT_AXIS_RX),
            pc.getAxisValue(AMOTION_EVENT_AXIS_RY)
        );

        if (!valid_motion_event(mir_event->motion))
            return;
        dispatcher->dispatch(*mir_event);
    }
    else
    {
        auto mir_event = mev::make_event(MirInputDeviceId(args->deviceId),
                                         args->eventTime,
                                         mia::mir_modifiers_from_android(args->metaState));
        auto action = args->action;
        size_t index_with_action = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        auto masked_action = action & AMOTION_EVENT_ACTION_MASK;
        for (unsigned i = 0; i < args->pointerCount; i++)
        {
            auto action = (i == index_with_action) ? mia::mir_touch_action_from_masked_android(masked_action) :
                mir_touch_action_change;
            mev::add_touch(*mir_event, args->pointerProperties[i].id, action,
                           mia::mir_tool_type_from_android(args->pointerProperties[i].toolType),
                           args->pointerCoords[i].getX(),
                           args->pointerCoords[i].getY(),
                           args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_PRESSURE),
                           args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MAJOR),
                           args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_TOUCH_MINOR),
                           args->pointerCoords[i].getAxisValue(AMOTION_EVENT_AXIS_SIZE));
        }

        if (!valid_motion_event(mir_event->motion))
            return;
        dispatcher->dispatch(*mir_event);
    }
}

void mia::InputTranslator::notifySwitch(const droidinput::NotifySwitchArgs* /*args*/)
{
    // TODO cannot be expressed through MirEvent
}

void mia::InputTranslator::notifyDeviceReset(const droidinput::NotifyDeviceResetArgs* args)
{
    MirEvent mir_event;
    mir_event.type = mir_event_type_input_configuration;
    auto& idev = mir_event.input_configuration;
    idev.action = mir_input_configuration_action_device_reset;
    idev.when = args->eventTime;
    idev.id = args->deviceId;

    dispatcher->dispatch(mir_event);
}

