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

#include <map>
#include <string>
#include <vector>

namespace mir {
namespace input {

/**
 * The mode of a position or axis of an input device
 */
enum class Mode {
    none, /**< This position is not supported */
    relative, /**< This position or axis provides relative values */
    absolute /**< This position or axis provides absolute values */
};

/**
 * Information on the position values of an input device
 */
struct PositionInfo {
    /**
     * The mode of the position
     */
    Mode mode;

    union {
        struct {
            struct {
                /**
                 * The maximum value of the position
                 */
                int max;

                /**
                 * The length of the position axis in millimeters
                 *
                 * A length of zero means the length is unknown.
                 */
                float length;
            } x, y;
        } relative; /**< Relative mode position information */
        struct {
            struct {
                /**
                 * The minimum screen coordinate of the surface
                 */
                int min;

                /**
                 * The maximum screen coordinate of the surface
                 */
                int max;
            } x, y;
        } absolute; /**< Absolute mode position information */
    };
};

/**
 * The axis types recognized by mir
 */
enum class AxisType {
    /**
     * Vertical scroll axis
     *
     * The resolution is measured in millimeters per unit.
     */
    vertical_scroll,
    /**
     * Horizontal scroll axis
     *
     * The resolution is measured in millimeters per unit.
     */
    horizontal_scroll
};

/**
 * Information on an axis of an input device
 */
struct Axis {
    /**
     * The mode of the axis
     *
     * May not be kNone.
     */
    Mode mode;

    /**
     * The minimum value of the axis
     */
    int min;

    /**
     * The maximum value of the axis
     */
    int max;

    /**
     * The resolution of the axis in axis-specific units
     */
    float resolution;
};

/**
 * A logical input device
 */
class LogicalDevice {
 public:
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
     * A bitmask of the buttons supported by the device
     *
     * @return The buttons supported by the device
     *
     * The bit positions are defined by the Linux evdev key code values.
     */
    virtual const std::vector<bool>& get_buttons() const = 0;

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
     */
    virtual const std::map<AxisType, Axis>& get_axes() const = 0;
};

} // input
} // mir

#endif // MIR_INPUT_LOGICAL_DEVICE_H_
