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
 */

#include "mir_test_framework/command_line_server_configuration.h"
#include "mir/options/default_configuration.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>

namespace mo=mir::options;

namespace
{
int argc;
char const** argv;
}

namespace mir_test_framework
{
    auto configuration_from_commandline()
    -> std::shared_ptr<mo::DefaultConfiguration>
    {
        return std::make_shared<mo::DefaultConfiguration>(::argc, ::argv);
    }
}

int main(int argc, char** argv)
{
    // Override this standard gtest message
    std::cout << "Running main() from " << basename(__FILE__) << std::endl;
    ::argc = std::remove_if(
        argv,
        argv+argc,
        [](char const* arg) { return !strncmp(arg, "--gtest_", 8); }) - argv;
    ::argv = const_cast<char const**>(argv);

    ::testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
