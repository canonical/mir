/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_MODULE_PROPERTIES_H_
#define MIR_PLATFORM_MODULE_PROPERTIES_H_

namespace mir
{

/**
 * Describes a platform module. Mir provides graphics platforms named "mir:*"
 * Mir provides a "mir:evdev-input" input platform.
 *
 * Third party platforms should be named according to the vendor and platform:
 *  "<vendor>:<platform>"
 */
struct ModuleProperties
{
    char const* name;
    int major_version;
    int minor_version;
    int micro_version;
    char const* file;
};
}

#endif /* MIR_PLATFORM_MODULE_PROPERTIES_H_ */
