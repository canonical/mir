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

#include "src/include/client/mir/client_platform_factory.h"
#include "src/include/client/mir/client_context.h"

#include "mir_test_framework/stub_client_platform_factory.h"
#include "mir_test_framework/stub_platform_helpers.h"
#include <memory>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace mcl = mir::client;

std::shared_ptr<mcl::ClientPlatform> create_client_platform(mcl::ClientContext* context)
{
    return mtf::StubClientPlatformFactory{}.create_client_platform(context);
}

bool is_appropriate_module(mcl::ClientContext* context)
{
    using namespace testing;
    MirPlatformPackage package;
    context->populate_server_package(package);
    return Matches(mtf::IsStubPlatformPackage())(package);
}
