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

#ifndef MIR_INPUT_INPUT_DEVICE_OBSERVER_H_
#define MIR_INPUT_INPUT_DEVICE_OBSERVER_H_

#include <memory>

namespace mir
{
namespace input
{
class Device;

class InputDeviceObserver
{
public:
    InputDeviceObserver() = default;
    virtual ~InputDeviceObserver() = default;

    virtual void device_added(std::shared_ptr<Device> const& device) = 0;
    virtual void device_changed(std::shared_ptr<Device> const& device) = 0;
    virtual void device_removed(std::shared_ptr<Device> const& device) = 0;
    /*!
     * Called after every group of changes.
     */
    virtual void changes_complete() = 0;

    InputDeviceObserver(InputDeviceObserver const&) = delete;
    InputDeviceObserver& operator=(InputDeviceObserver const&) = delete;
};

}
}

#endif
