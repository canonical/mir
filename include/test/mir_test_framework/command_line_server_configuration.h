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

#ifndef COMMAND_LINE_SERVER_CONFIGURATION
#define COMMAND_LINE_SERVER_CONFIGURATION

#include "mir/options/default_configuration.h"

namespace mo=mir::options;

namespace mir_test_framework
{
	auto configuration_from_commandline() 
	-> std::shared_ptr<mo::DefaultConfiguration>;
}

int main(int argc, char** argv);

#endif /* COMMAND_LINE_SERVER_CONFIGURATION */
