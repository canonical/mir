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

#include "mir_test_framework/connected_client_headless_server.h"

#include "boost/throw_exception.hpp"

namespace mtf = mir_test_framework;

void mtf::ConnectedClientHeadlessServer::SetUp()
{
    HeadlessInProcessServer::SetUp();
    connection = mir_connect_sync(new_connection().c_str(), __PRETTY_FUNCTION__);

    if (!mir_connection_is_valid(connection))
    {
        BOOST_THROW_EXCEPTION(
            std::runtime_error{std::string{"Failed to connect to test server: "} +
                               mir_connection_get_error_message(connection)});
    }
}

void mtf::ConnectedClientHeadlessServer::TearDown()
{
    mir_connection_release(connection);
    HeadlessInProcessServer::TearDown();
}
