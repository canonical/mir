/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_TEST_SERVER_H
#define MIRAL_TEST_SERVER_H

#include <miral/test_display_server.h>

#include <gtest/gtest.h>

namespace miral
{
struct TestServer : TestDisplayServer, testing::Test
{
    bool start_server_in_setup = true;

    // Start the server (unless start_server_in_setup is false)
    void SetUp() override;

    // Stop the server
    void TearDown() override;
};
}

#endif //MIRAL_TEST_SERVER_H
