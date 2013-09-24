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

#ifndef MIR_TESTING_CLIENT_CONFIGURATION
#define MIR_TESTING_CLIENT_CONFIGURATION

#include "mir_toolkit/mir_client_library_debug.h"
#include "src/client/default_connection_configuration.h"
#include <memory>

namespace mir
{
namespace options
{
class Option;
}
}

namespace mir_test_framework
{
struct StubConnectionConfiguration : public mir::client::DefaultConnectionConfiguration
{
    StubConnectionConfiguration(std::string const& socket_file);
    std::shared_ptr<mir::client::ClientPlatformFactory> the_client_platform_factory() override;
};

struct TestingClientConfiguration
{
    TestingClientConfiguration() {}
    virtual ~TestingClientConfiguration() {}

    // Code to run in client process
    virtual void exec() = 0;

};

}
#endif /* MIR_TESTING_CLIENT_CONFIGURATION */
