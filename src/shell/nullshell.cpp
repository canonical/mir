/*
 * Null Shell
 *
 * Copyright © 2013 Canonical Ltd.
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
 * Author: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "mir/shell/nullshell.h"

using namespace mir;
using namespace shell;

NullShell::~NullShell()
{
}

bool NullShell::supports(MirSurfaceAttrib attrib) const
{
    return (attrib == mir_surface_attrib_type);
}

bool NullShell::supports(MirSurfaceAttrib attrib, int value) const
{
    bool valid = false;

    switch (attrib)
    {
    case mir_surface_attrib_type:
        valid = (value >= 0 && value < mir_surface_type_arraysize_);
        break;
    default:
        break;
    }

    return valid;
}
