/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_TEST_FRAMEWORK_STUB_PLATFORM_HELPERS_H_
#define MIR_TEST_FRAMEWORK_STUB_PLATFORM_HELPERS_H_

#include "mir/graphics/platform_ipc_package.h"
#include "mir_toolkit/client_types.h"

#include <gmock/gmock.h>

namespace mir_test_framework
{
constexpr int stub_data_size{21};
constexpr int stub_data_guard{0x0eadbeef};

static inline void pack_stub_ipc_package(mir::graphics::PlatformIPCPackage& package)
{
    package.ipc_data = std::vector<int32_t>(stub_data_size, -1);
    package.ipc_data[0] = stub_data_guard;
}

static inline void create_stub_platform_package(MirPlatformPackage& package)
{
    ::memset(&package, 0, sizeof(package));
    package.data_items = stub_data_size;
    package.data[0] = stub_data_guard;
}

MATCHER(IsStubPlatformPackage, "")
{
    return (arg.data_items == stub_data_size) &&
           (arg.data[0] == stub_data_guard) &&
           (arg.fd_items == 0);
}
}

#endif // MIR_TEST_FRAMEWORK_STUB_PLATFORM_HELPERS_H_
