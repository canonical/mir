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

#ifndef MIR_TEST_FRAMEWORK_CLIENT_PLATFORM_FACTORY_H_
#define MIR_TEST_FRAMEWORK_CLIENT_PLATFORM_FACTORY_H_


#include "mir/shared_library.h"
#include "mir/client_platform_factory.h"
#include "mir_test_framework/executable_path.h"
#include "mir/test/doubles/mock_client_context.h"

namespace mtd = mir::test::doubles;

namespace mir_test_framework
{
std::shared_ptr<mir::SharedLibrary> platform_library;

std::shared_ptr<mir::client::ClientPlatform> create_android_client_platform()
{
    using namespace testing;
    mtd::MockClientContext ctx;
    ON_CALL(ctx, populate_server_package(_))
        .WillByDefault(Invoke([](MirPlatformPackage& package) { ::memset(&package, 0, sizeof(package)); }));
    platform_library = std::make_shared<mir::SharedLibrary>(client_platform("android"));
    auto platform_factory = platform_library->load_function<mir::client::CreateClientPlatform>("create_client_platform");
    return platform_factory(&ctx);
}

std::shared_ptr<mir::client::ClientPlatform> create_mesa_client_platform(
    mir::client::ClientContext* client_context)
{
    using namespace testing;
    platform_library = std::make_shared<mir::SharedLibrary>(client_platform("mesa"));
    auto platform_factory = platform_library->load_function<mir::client::CreateClientPlatform>("create_client_platform");
    return platform_factory(client_context);
}

std::shared_ptr<mir::SharedLibrary>
get_platform_library()
{
    if (!platform_library)
    {
        throw std::logic_error{"Must call one of create_*_client_platform() before calling get_platform_library()"};
    }
    return platform_library;
}

}

#endif // MIR_TEST_FRAMEWORK_CLIENT_PLATFORM_FACTORY_H_
