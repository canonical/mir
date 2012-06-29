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
 * Authored by:
 *   Chase Douglas <chase.douglas@canonical.com>
 *   Thomas Voss  <thomas.voss@canonical.com>
 */

#ifndef MIR_INPUT_AXIS_H_
#define MIR_INPUT_AXIS_H_

#include <iosfwd>

namespace mir
{
namespace input
{

/**
 * The mode of a position or axis of an input device
 */
enum class Mode {
    none, /**< This position is not supported */
    relative, /**< This position or axis provides relative values */
    absolute /**< This position or axis provides absolute values */
};

// Only here to make tests using Google mock and mode compile.
std::ostream& operator<<(std::ostream& out, const Mode& m);

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
    // FIXME(Reenable once under test)
    // Mode mode;

    /**
     * The minimum value of the axis
     */
    // FIXME(Reenable once under test)
    // int min;

    /**
     * The maximum value of the axisa
     */
    // FIXME(Reenable once under test)
    // int max;

    /**
     * The resolution of the axis in axis-specific units
     */
    // FIXME(Reenable once under test)
    // float resolution;
};

// Only here to make tests using Google mock and Axis compile.
std::ostream& operator<<(std::ostream& out, const Axis& a);

}
}

#endif // MIR_INPUT_AXIS_H_
