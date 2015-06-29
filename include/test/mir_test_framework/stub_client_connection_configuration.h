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

#ifndef MIR_TESTING_STUB_CLIENT_CONNECTION_CONFIGURATION
#define MIR_TESTING_STUB_CLIENT_CONNECTION_CONFIGURATION

#include "src/client/default_connection_configuration.h"
#include <memory>

namespace mir_test_framework
{

struct StubConnectionConfiguration : public mir::client::DefaultConnectionConfiguration
{
    StubConnectionConfiguration(std::string const& socket_file);
    std::shared_ptr<mir::client::ClientPlatformFactory> the_client_platform_factory() override;
};

}
#endif /* MIR_TESTING_STUB_CLIENT_CONNECTION_CONFIGURATION */
