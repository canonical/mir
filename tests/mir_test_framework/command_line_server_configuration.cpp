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
#include "mir_test_framework/main.h"

#include "mir/options/default_configuration.h"
#include "mir/server.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cstring>

namespace mo=mir::options;

namespace
{
char const* args[] = {nullptr};
int argc;
char const** argv = args;
}

auto mir_test_framework::configuration_from_commandline()
-> std::shared_ptr<mo::DefaultConfiguration>
{
    return std::make_shared<mo::DefaultConfiguration>(::argc, ::argv);
}

void mir_test_framework::configure_from_commandline(mir::Server& server)
{
    server.set_command_line(::argc, ::argv);
}

void mir_test_framework::set_commandline(int argc, char* argv[])
{
    ::argv = const_cast<char const**>(argv);
    ::argc = argc;
}

int mir_test_framework::main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    set_commandline(argc, argv);
    return RUN_ALL_TESTS();
}

