/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_test_framework/stub_client_connection_configuration.h"
#include "src/client/default_connection_configuration.h"
#include "mir_test_framework/stub_client_platform_factory.h"

namespace mcl = mir::client;
namespace mtf = mir_test_framework;

mtf::StubConnectionConfiguration::StubConnectionConfiguration(std::string const& socket_file)
    : DefaultConnectionConfiguration(socket_file)
{
}

std::shared_ptr<mcl::ClientPlatformFactory> mtf::StubConnectionConfiguration::the_client_platform_factory()
{
    return std::make_shared<StubClientPlatformFactory>();
}
