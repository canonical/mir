/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_PLATFORM_MODULE_PROPERTIES_H_
#define MIR_PLATFORM_MODULE_PROPERTIES_H_

#include "mir/flags.h"

namespace mir
{

enum class ModuleType : uint32_t
{
    server_graphics_platform = 1,
    server_input_platform = server_graphics_platform << 1,
};

void mir_enable_enum_bit_operators(ModuleType);

/**
 * Describes a platform module
 */
struct ModuleProperties
{
    char const* name;
    mir::Flags<ModuleType> type;
    int major_version;
    int minor_version;
    int micro_version;
};

extern "C" typedef ModuleProperties const*(*DescribeModule)();
extern "C" ModuleProperties const* describe_module();
}

#endif /* MIR_PLATFORM_MODULE_PROPERTIES_H_ */
