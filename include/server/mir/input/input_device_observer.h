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

#ifndef MIR_INPUT_INPUT_DEVICE_OBSERVER_H_
#define MIR_INPUT_INPUT_DEVICE_OBSERVER_H_

namespace mir
{
namespace input
{
class InputDeviceInfo;

class InputDeviceObserver
{
public:
    InputDeviceObserver() = default;
    virtual ~InputDeviceObserver() = default;

    virtual void device_added(InputDeviceInfo const& device) = 0;
    virtual void device_changed(InputDeviceInfo const& device) = 0;
    virtual void device_removed(InputDeviceInfo const& device) = 0;
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
