/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_EVENT_CONVERSION_HELPERS_H_
#define MIR_INPUT_EVENT_CONVERSION_HELPERS_H_

#include "mir_toolkit/event.h"

namespace mir
{
namespace input
{
namespace android
{
MirInputEventModifiers mir_modifiers_from_android(int32_t android_modifiers);
int32_t android_modifiers_from_mir(MirInputEventModifiers modifiers);

MirKeyboardAction mir_keyboard_action_from_android(int32_t android_action, int32_t repeat_count);

// Mir differentiates between mir_keyboard_action_down
// and mir_keyboard_action_repeat whereas android encodes
// keyrepeats as AKEY_EVENT_ACTION_DOWN and a repeatCount of > 0
// Thus when converting from MirKeyboardAction to an android
// action we must also fetch a repeat count for the android event.
int32_t android_keyboard_action_from_mir(int32_t& repeat_count_out, MirKeyboardAction action);
}
}
}

#endif // MIR_INPUT_EVENT_CONVERSION_HELPERS_H_
