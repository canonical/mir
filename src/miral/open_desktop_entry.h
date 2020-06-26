/*
 * Copyright © 2019 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef EGMDE_OPEN_DESKTOP_ENTRY_H
#define EGMDE_OPEN_DESKTOP_ENTRY_H

#include <string>
#include <vector>

namespace egmde
{
void open_desktop_entry(std::string const& desktop_file, std::vector<std::string> const& env);
}

#endif //EGMDE_OPEN_DESKTOP_ENTRY_H
