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

// TODO: unit tests for these

NullShell::~NullShell()
{
}

bool NullShell::start()
{
    return true;
}

void NullShell::stop()
{
}

bool NullShell::supports(MirSurfaceAttrib attrib) const
{
    return (attrib == MIR_SURFACE_TYPE);
}

bool NullShell::supports(MirSurfaceAttrib attrib, int value) const
{
    // What really makes sense here? Everything?
    return (attrib == MIR_SURFACE_TYPE) &&
                      ((value == MIR_SURFACE_NORMAL) ||
                       (value == MIR_SURFACE_FREESTYLE));
}
