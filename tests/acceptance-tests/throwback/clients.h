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
 */

#ifndef MIR_TESTS_ACCEPTANCE_TESTS_CLIENTS_H_
#define MIR_TESTS_ACCEPTANCE_TESTS_CLIENTS_H_

#include "mir_toolkit/mir_client_library.h"
#include "mir_test_framework/testing_client_configuration.h"

namespace mir_test_framework
{

extern char const* const mir_test_socket;

struct SurfaceCreatingClient : TestingClientConfiguration
{
    void exec();
    MirConnection* connection;
    MirSurface* surface;
};

}

#endif
