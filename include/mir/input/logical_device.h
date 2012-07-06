/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Chase Douglas <chase.douglas@canonical.com>
 */

#ifndef MIR_INPUT_LOGICAL_DEVICE_H_
#define MIR_INPUT_LOGICAL_DEVICE_H_

#include "mir/input/axis.h"
#include "mir/input/event_producer.h"

#include <map>
#include <stdexcept>
#include <string>

namespace mir
{
namespace input
{

struct PositionInfo;

/* FIXME: Reenable once under test.
enum Button
{
    button_none = 0
};
*/

struct NoAxisForTypeException : public std::runtime_error
{
    explicit NoAxisForTypeException() : std::runtime_error("Missing axis for type")
    {
    }
};

/**
 * A logical input device
 */
class LogicalDevice : public EventProducer {
 public:

    explicit LogicalDevice(EventHandler* event_handler) : EventProducer(event_handler)
    {
    }

    virtual ~LogicalDevice() {}

    /**
     * The name of the logical device
     *
     * @return The name of the device
     *
     * Examples: "Touch Input", "Pen Input"
     */
    virtual const std::string& get_name() const = 0;

    /**
     * The number of simultaneous instances
     *
     * @return The number of simultaneous instances
     *
     * For example, a ten touch device would have ten simultaneous touch
     * instances.
     */
    virtual int get_simultaneous_instances() const = 0;

    /**
     * Queries if a button is supported by the device.
     *
     * @return true if the button is supported by the device, false otherwise.
     *
     */
    // FIXME: Reenable once under test.
    // virtual bool is_button_supported(const Button& button) const = 0;

    /**
     * The position info
     *
     * @return The position info
     */
    virtual const mir::input::PositionInfo& get_position_info() const = 0;

    /**
     * The axis info
     *
     * @return The info for the axes of the device
     *
     * @throw Throws NoAxisForTypeException if device does not support
     * the axis type.
     */
    virtual const std::map<AxisType, Axis>& get_axes() const = 0;
};

} // input
} // mir

#endif // MIR_INPUT_LOGICAL_DEVICE_H_
