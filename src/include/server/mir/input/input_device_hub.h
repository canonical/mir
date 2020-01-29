/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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

#ifndef MIR_INPUT_INPUT_DEVICE_HUB_H_
#define MIR_INPUT_INPUT_DEVICE_HUB_H_

#include <memory>
#include <functional>

namespace mir
{
namespace input
{
class Device;
class InputDeviceInfo;
class InputDeviceObserver;

class InputDeviceHub
{
public:
    InputDeviceHub() = default;
    virtual ~InputDeviceHub() = default;

    virtual void add_observer(std::shared_ptr<InputDeviceObserver> const&) = 0;
    virtual void remove_observer(std::weak_ptr<InputDeviceObserver> const&) = 0;
    virtual void for_each_input_device(std::function<void(Device const& device)> const& callback) = 0;
    virtual void for_each_mutable_input_device(std::function<void(Device& device)> const& callback) = 0;

    InputDeviceHub(InputDeviceHub const&) = delete;
    InputDeviceHub& operator=(InputDeviceHub const&) = delete;
};

}
}

#endif
