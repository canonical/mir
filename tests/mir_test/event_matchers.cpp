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

#include "mir/test/event_matchers.h"

#include "mir/event_printer.h"

void PrintTo(MirEvent const& event, std::ostream *os)
{
    using mir::operator<<;
    *os << event;
}

void PrintTo(MirEvent const* event, std::ostream *os)
{
    if (event)
        PrintTo(*event, os);
    else
        *os << "nullptr";
}
