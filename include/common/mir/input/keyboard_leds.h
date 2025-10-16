/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_KEYBOARD_LEDS_H
#define MIR_INPUT_KEYBOARD_LEDS_H

#include <mir/flags.h>
#include <cstdint>

namespace mir
{
namespace input
{
enum class KeyboardLed : uint32_t
{
    caps_lock = (1 << 0),
    num_lock = (1 << 1),
    scroll_lock = (1 << 2)
};

KeyboardLed mir_enable_enum_bit_operators(KeyboardLed);
using KeyboardLeds = mir::Flags<KeyboardLed>;

}
}

#endif //MIR_INPUT_KEYBOARD_LEDS_H
