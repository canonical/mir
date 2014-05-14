/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Josh Arenson <joshua.arenson@canonical.com>
 */

#include "mir/default_configuration.h"
#include "mir/options/default_configuration.h"
#include "mir/default_server_configuration.h"
#include "mir_test_framework/command_line_server_configuration.h"

#include <boost/algorithm/string.hpp>
#include <gtest/gtest.h>

#include <algorithm>
#include <memory>

namespace mo=mir::options;

int argc;
char const** argv;

namespace mir_test_framework
{
	using namespace std;
	auto configuration_from_commandline()
	-> shared_ptr<mo::Configuration>
	{
	  return make_shared<mo::DefaultConfiguration>(::argc, ::argv);
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



