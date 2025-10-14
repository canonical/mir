/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_INPUT_NULL_INPUT_DEVICE_OBSERVER_H_
#define MIR_INPUT_NULL_INPUT_DEVICE_OBSERVER_H_

#include <mir/input/input_device_observer.h>

namespace mir
{
namespace input
{

class NullInputDeviceObserver : public InputDeviceObserver
{
public:
    void device_added(std::shared_ptr<Device> const&) override {}
    void device_changed(std::shared_ptr<Device> const&) override {}
    void device_removed(std::shared_ptr<Device> const&) override {}
    void changes_complete() override {}

    ~NullInputDeviceObserver() override = default;
};

}
}

#endif
