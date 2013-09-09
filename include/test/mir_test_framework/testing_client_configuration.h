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

struct TestingClientConfiguration
{
    TestingClientConfiguration();
    virtual ~TestingClientConfiguration();

    // Code to run in client process
    virtual void exec() = 0;

    void set_client_configuration(std::shared_ptr<mir::options::Option> const& options);

    //force client configuration to use default connect and disconnect functions
    void use_default_connect_functions();

private:
    MirWaitHandle* (*default_mir_connect_impl)
        (char const *server, char const *app_name, mir_connected_callback callback, void *context);

    void (*default_mir_connection_release_impl) (MirConnection *connection);
};

}
#endif /* MIR_TESTING_CLIENT_CONFIGURATION */
