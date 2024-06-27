/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_INPUT_LED_OBSERVER_REGISTER_H
#define MIR_INPUT_LED_OBSERVER_REGISTER_H

#include "mir/observer_registrar.h"
#include "mir/input/keyboard_leds.h"
#include "mir_toolkit/mir_input_device_types.h"

namespace mir
{
namespace input
{
class LedObserver
{
public:
    virtual ~LedObserver() = default;
    virtual void leds_set(MirInputDeviceId id, KeyboardLeds leds) = 0;
};

class LedObserverRegistrar : public ObserverRegistrar<LedObserver>
{
public:
    virtual ~LedObserverRegistrar() = default;
};
}
}

#endif //MIR_INPUT_LED_OBSERVER_REGISTER_H
