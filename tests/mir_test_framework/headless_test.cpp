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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/headless_test.h"

namespace mtf = mir_test_framework;

mtf::HeadlessTest::HeadlessTest()
{
    add_to_environment("MIR_SERVER_PLATFORM_GRAPHICS_LIB", "libmirplatformstub.so");
}

void mtf::HeadlessTest::add_to_environment(char const* key, char const* value)
{
    env.emplace_back(key, value);
}

auto mtf::HeadlessTest::new_connection() -> std::string
{
    char connect_string[64] = {0};
    sprintf(connect_string, "fd://%d", server.open_client_socket());
    return connect_string;
}
