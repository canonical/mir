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

#include "mir/input/position_info.h"

#include <boost/format.hpp>

#include <ostream>

namespace mi = mir::input;

namespace mir
{
namespace input
{

std::ostream& operator<<(std::ostream& out, const mi::PositionInfo& pi)
{
    out <<
            (boost::format("") % pi.mode).str();
    return out;
}

}
}
