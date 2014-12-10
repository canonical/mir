/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "clients.h"
#include "mir_test_framework/testing_process_manager.h"
#include "mir_test_framework/any_surface.h"

#include <gtest/gtest.h>

namespace mtf = mir_test_framework;

char const* const mtf::mir_test_socket = mtf::test_socket_file().c_str();

void mtf::SurfaceCreatingClient::exec()
{
    connection = mir_connect_sync(
        mir_test_socket,
        __PRETTY_FUNCTION__);
    ASSERT_TRUE(connection != NULL);
    surface = mtf::make_any_surface(connection);
    mir_connection_release(connection);
}
