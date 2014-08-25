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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_test_framework/using_stub_client_platform.h"
#include "mir_test_framework/stub_client_connection_configuration.h"
#include "mir_toolkit/mir_client_library.h"
#include "src/client/mir_connection_api.h"

namespace mtf = mir_test_framework;
namespace mcl = mir::client;

namespace
{

void null_lifecycle_callback(MirConnection*, MirLifecycleState, void*)
{
}

class StubMirConnectionAPI : public mcl::MirConnectionAPI
{
public:
    StubMirConnectionAPI(mcl::MirConnectionAPI* prev_api)
        : prev_api{prev_api}
    {
    }

    MirWaitHandle* connect(
        char const* socket_file,
        char const* name,
        mir_connected_callback callback,
        void* context) override
    {
        return prev_api->connect(socket_file, name, callback, context);
    }

    void release(MirConnection* connection) override
    {
        // Clear the lifecycle callback in order not to get SIGTERM by the default
        // lifecycle handler during connection teardown
        mir_connection_set_lifecycle_event_callback(connection, null_lifecycle_callback, nullptr);
        return prev_api->release(connection);
    }

    std::unique_ptr<mcl::ConnectionConfiguration> configuration(std::string const& socket) override
    {
        return std::unique_ptr<mcl::ConnectionConfiguration>{
            new mtf::StubConnectionConfiguration{socket}};
    }

private:
    mcl::MirConnectionAPI* const prev_api;
};

}

mtf::UsingStubClientPlatform::UsingStubClientPlatform()
    : prev_api{mir_connection_api_impl},
      stub_api{new StubMirConnectionAPI{prev_api}}
{
    mir_connection_api_impl = stub_api.get();
}

mtf::UsingStubClientPlatform::~UsingStubClientPlatform()
{
    mir_connection_api_impl = prev_api;
}
