/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>>
 */

#include "mir/graphics/platform.h"

extern "C" mir::graphics::PlatformPriority probe_platform()
{
    return mir::graphics::supported;
}

mir::graphics::ModuleProperties const description {
    "dummy"
};

extern "C" mir::graphics::ModuleProperties const* describe_module()
{
    return &description;
}
