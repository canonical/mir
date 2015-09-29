/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_configuration.h"

#include <cstdlib>
#include <sstream>

char const* mir::default_server_socket()
{
    std::ostringstream formatter;

    char const* dir = getenv("XDG_RUNTIME_DIR");
    if (!dir) dir = "/tmp";

    formatter << dir << "/mir_socket";

    static auto result = formatter.str();
    return result.c_str();
}
