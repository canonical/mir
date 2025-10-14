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

#include <mir/input/keyboard_leds.h>
#include <mir_toolkit/mir_input_device_types.h>
#include <memory>

namespace mir
{
namespace input
{
class LedObserver
{
public:
    virtual ~LedObserver() = default;
    virtual void leds_set(KeyboardLeds leds) = 0;
};

class LedObserverRegistrar
{
public:
    LedObserverRegistrar() = default;
    virtual ~LedObserverRegistrar() = default;

    virtual void register_interest(std::weak_ptr<LedObserver> const& observer, MirInputDeviceId id) = 0;
    virtual void unregister_interest(LedObserver const& observer, MirInputDeviceId id) = 0;

protected:
    LedObserverRegistrar(LedObserverRegistrar const&) = delete;
    LedObserverRegistrar& operator=(LedObserverRegistrar const&) = delete;
};
}
}

#endif //MIR_INPUT_LED_OBSERVER_REGISTER_H
