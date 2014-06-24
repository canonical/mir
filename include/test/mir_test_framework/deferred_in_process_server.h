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

#ifndef MIR_TEST_FRAMEWORK_DEFERRED_IN_PROCESS_SERVER_H_
#define MIR_TEST_FRAMEWORK_DEFERRED_IN_PROCESS_SERVER_H_

#include "mir_test_framework/server_runner.h"

#include <gtest/gtest.h>

namespace mir_test_framework
{
/** Fixture for running Mir server in test process.
 * The server startup is deferred until start_server() is called, to allows the
 * test code to initialize the server environment with expectations or stubs.
 */
struct DeferredInProcessServer : testing::Test, private ServerRunner
{
    void TearDown() override { ServerRunner::stop_server(); }

    using ServerRunner::start_server;
    using ServerRunner::new_connection;
};
}

#endif /* MIR_TEST_FRAMEWORK_DEFERRED_IN_PROCESS_SERVER_H_ */
