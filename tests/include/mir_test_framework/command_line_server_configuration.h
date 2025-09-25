/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_TEST_FRAMEWORK_COMMAND_LINE_SERVER_CONFIGURATION
#define MIR_TEST_FRAMEWORK_COMMAND_LINE_SERVER_CONFIGURATION

#include <memory>

namespace mir
{
namespace options
{
class DefaultConfiguration;
}
class Server;
}

namespace mir_test_framework
{
    auto configuration_from_commandline()
    -> std::shared_ptr<mir::options::DefaultConfiguration>;

    void configure_from_commandline(mir::Server& server);
}

#endif /* MIR_TEST_FRAMEWORK_COMMAND_LINE_SERVER_CONFIGURATION */
