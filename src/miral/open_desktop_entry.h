/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIRAL_OPEN_DESKTOP_ENTRY_H
#define MIRAL_OPEN_DESKTOP_ENTRY_H

#include <string>
#include <vector>

namespace miral
{
void open_desktop_entry(std::string const& desktop_file);
}

#endif //MIRAL_OPEN_DESKTOP_ENTRY_H
