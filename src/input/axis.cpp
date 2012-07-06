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

#include "mir/input/axis.h"

#include <boost/format.hpp>

#include <ostream>
#include <string>

namespace mi = mir::input;

namespace mir
{
namespace input
{

std::ostream& operator<<(std::ostream& out, const mi::Mode& m)
{
    switch (m)
    {
        case mi::Mode::none: out << "none";
            break;
        case mi::Mode::relative: out << "relative";
            break;
        case mi::Mode::absolute: out << "absolute";
            break;
    }
    return out;
}

std::ostream& operator<<(std::ostream& out, const mi::Axis& /*a*/)
{
    // FIXME(Reenable once Axis is under test)
    /*static const std::string print_format("Axis[mode: %1, min: %2, max: %3, resolution: %4]");
    out <<
    (boost::format(print_format) % a.mode % a.min % a.max %a.resolution).str();
    */

    return out;
}

}
}
