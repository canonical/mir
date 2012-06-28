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
 *  Chase Douglas <chase.douglas@canonical.com>
 *  Thomas Voss <thomas.voss@canonical.com>
 */

#ifndef MIR_INPUT_POSITION_INFO_H_
#define MIR_INPUT_POSITION_INFO_H_

#include "mir/input/axis.h"

#include <iosfwd>

namespace mir
{
namespace input
{

/**
 * Information on the position values of an input device
 */
struct PositionInfo {
    /**
     * The mode of the position
     */
    Mode mode;
};

std::ostream& operator<<(std::ostream& out, const PositionInfo& pi);

}
}

#endif // MIR_INPUT_POSITION_INFO_H_
