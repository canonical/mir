/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/display_server_test_fixture.h"
#include "mir_test_framework/testing_client_configuration.h"
#include "src/client/mir_connection.h"

namespace mc = mir::compositor;
namespace mtf = mir_test_framework;

/* if set before any calls to the api functions, assigning to this pointer will allow user to
 * override calls to mir_connect() and mir_connection_release(). This is mostly useful in test scenarios
 */
extern MirWaitHandle* (*mir_connect_impl)(
    char const *server,
    char const *app_name,
    mir_connected_callback callback,
    void *context);
extern void (*mir_connection_release_impl) (MirConnection *connection);

namespace
{
MirWaitHandle* mir_connect_test_override(
    char const *socket_file,
    char const *app_name,
    mir_connected_callback callback,
    void *context)
{
    mtf::StubConnectionConfiguration conf(socket_file);
    auto connection = new MirConnection(conf);
    return connection->connect(app_name, callback, context);
}

void mir_connection_release_override(MirConnection *connection)
{
    auto wait_handle = connection->disconnect();
    wait_handle->wait_for_all();
    delete connection;
}
}

mtf::TestingProcessManager mir_test_framework::DefaultDisplayServerTestFixture::process_manager;
mtf::TestingServerConfiguration mir_test_framework::DefaultDisplayServerTestFixture::default_parameters;

DefaultDisplayServerTestFixture::DefaultDisplayServerTestFixture()
{
    default_mir_connect_impl = mir_connect_impl;
    default_mir_connection_release_impl = mir_connection_release_impl;

    auto options = default_parameters.the_options();
    if (!options->get("tests-use-real-graphics", false))
    {
        mir_connect_impl = mir_connect_test_override;
        mir_connection_release_impl = mir_connection_release_override;
    }
}

void DefaultDisplayServerTestFixture::launch_client_process(TestingClientConfiguration& config)
{
    process_manager.launch_client_process(config);
}

void DefaultDisplayServerTestFixture::SetUpTestCase()
{
    process_manager.launch_server_process(default_parameters);
}


void DefaultDisplayServerTestFixture::TearDown()
{
    process_manager.tear_down_clients();
}

void DefaultDisplayServerTestFixture::TearDownTestCase()
{
    process_manager.tear_down_server();
}

void DefaultDisplayServerTestFixture::use_default_connect_functions()
{
    mir_connect_impl = default_mir_connect_impl;
    mir_connection_release_impl = default_mir_connection_release_impl; 
}

DefaultDisplayServerTestFixture::~DefaultDisplayServerTestFixture()
{
    use_default_connect_functions();
}


void BespokeDisplayServerTestFixture::launch_server_process(TestingServerConfiguration& functor)
{
    server_options = functor.the_options();
    process_manager.launch_server_process(functor);
    default_mir_connect_impl = mir_connect_impl;
    default_mir_connection_release_impl = mir_connection_release_impl;
}

void BespokeDisplayServerTestFixture::launch_client_process(TestingClientConfiguration& config)
{
    if (!server_options->get("tests-use-real-graphics", false) && !use_default_fns)
    {
        mir_connect_impl = mir_connect_test_override;
        mir_connection_release_impl = mir_connection_release_override;
    }
    process_manager.launch_client_process(config);
}

bool BespokeDisplayServerTestFixture::shutdown_server_process()
{
    // TODO fix problem and remove this frig.
    // problem: sometimes the server exits normally with status
    // EXIT_SUCCESS but the test process sees TerminationReason::unknown
    auto const& result = process_manager.shutdown_server_process();
    return result.succeeded()
        || result.reason == TerminationReason::unknown;
}

void BespokeDisplayServerTestFixture::terminate_client_processes()
{
    process_manager.terminate_client_processes();
}


void BespokeDisplayServerTestFixture::kill_client_processes()
{
    process_manager.kill_client_processes();
}

void BespokeDisplayServerTestFixture::run_in_test_process(
    std::function<void()> const& run_code)
{
    process_manager.run_in_test_process(run_code);
}

void BespokeDisplayServerTestFixture::SetUp()
{
}

void BespokeDisplayServerTestFixture::TearDown()
{
    process_manager.tear_down_all();
}

BespokeDisplayServerTestFixture::BespokeDisplayServerTestFixture()
    : use_default_fns(false)
{
}

BespokeDisplayServerTestFixture::~BespokeDisplayServerTestFixture()
{
    mir_connect_impl = default_mir_connect_impl;
    mir_connection_release_impl = default_mir_connection_release_impl; 
}

void BespokeDisplayServerTestFixture::use_default_connect_functions()
{
    use_default_fns = true;
}
