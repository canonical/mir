/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_TEST_FRAMEWORK_USING_CLIENT_PLATFORM_H
#define MIR_TEST_FRAMEWORK_USING_CLIENT_PLATFORM_H

#include <memory>
#include <functional>

#include "mir_test_framework/stub_client_connection_configuration.h"
#include "src/client/mir_connection_api.h"

namespace mcl = mir::client;

namespace mir_test_framework
{

class StubMirConnectionAPI : public mcl::MirConnectionAPI
{
public:
    StubMirConnectionAPI(
        mcl::MirConnectionAPI* prev_api,
        mcl::ConfigurationFactory factory)
        : prev_api{prev_api},
          factory{factory}
    {
    }

    MirWaitHandle* connect(
        mcl::ConfigurationFactory configuration,
        char const* socket_file,
        char const* name,
        mir_connected_callback callback,
        void* context) override;

    void release(MirConnection* connection) override;

    mcl::ConfigurationFactory configuration_factory() override;

private:
    mcl::MirConnectionAPI* const prev_api;
    mcl::ConfigurationFactory factory;
};

template<class ClientConfig>
class UsingClientPlatform
{
public:
    UsingClientPlatform()
        : prev_api{mir_connection_api_impl},
          stub_api{
              new StubMirConnectionAPI{prev_api, make_configuration_factory()}}
    {
        mir_connection_api_impl = stub_api.get();
    }

    ~UsingClientPlatform()
    {
        mir_connection_api_impl = prev_api;
    }

private:
    UsingClientPlatform(UsingClientPlatform const&) = delete;

    UsingClientPlatform operator=(UsingClientPlatform const&) = delete;

    mir::client::MirConnectionAPI* prev_api;
    std::unique_ptr<mir::client::MirConnectionAPI> stub_api;

    mcl::ConfigurationFactory make_configuration_factory()
    {
        return [](std::string const& socket)
            {
            return std::unique_ptr<mir::client::ConnectionConfiguration>{
                new ClientConfig{socket}
            };
            };
    }
};
}

#endif //MIR_TEST_FRAMEWORK_USING_CLIENT_PLATFORM_H
