/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_INPUT_EVDEV_INPUT_MODIFIER_UTILS_H_
#define MIR_INPUT_EVDEV_INPUT_MODIFIER_UTILS_H_

#include "mir_toolkit/event.h"

namespace mir
{
namespace input
{
namespace evdev
{
MirPointerButton to_pointer_button(int button);
MirInputEventModifier to_modifier(int32_t scan_code);
MirInputEventModifiers expand_modifiers(MirInputEventModifiers modifiers);
}
}
}

#endif
