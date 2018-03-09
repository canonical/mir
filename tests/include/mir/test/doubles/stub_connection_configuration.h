/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com> 
 */

#ifndef MIR_TEST_DOUBLES_STUB_CONNECTION_CONFIGURATION_H_
#define MIR_TEST_DOUBLES_STUB_CONNECTION_CONFIGURATION_H_

#include "src/client/default_connection_configuration.h"
#include "mir/dispatch/dispatchable.h"
#include "mir_test_framework/client_platform_factory.h"
namespace mir
{
namespace test
{
namespace doubles
{
struct StubConnectionConfiguration : client::DefaultConnectionConfiguration
{
    struct DummyChannel : client::rpc::MirBasicRpcChannel,
        dispatch::Dispatchable
    {
        void call_method(
            std::string const&,
            google::protobuf::MessageLite const*,
            google::protobuf::MessageLite*,
            google::protobuf::Closure* c) override
        {
            channel_call_count++;
            c->Run();
        }
        void discard_future_calls() override
        {
        }
        void wait_for_outstanding_calls() override
        {
        }
        mir::Fd watch_fd() const override
        {
            int fd[2];
            if (pipe(fd))
                return {};
            mir::Fd{fd[1]};
            return mir::Fd{fd[0]};
        }
        bool dispatch(mir::dispatch::FdEvents) override
        {
            return true;
        }
        mir::dispatch::FdEvents relevant_events() const override { return {}; }
        int channel_call_count = 0;
    };

    StubConnectionConfiguration(
        std::shared_ptr<client::ClientPlatform> const& platform) :
        DefaultConnectionConfiguration(""),
        channel(std::make_shared<DummyChannel>()),
        platform(platform)
    {
    }

    std::shared_ptr<client::ClientPlatformFactory> the_client_platform_factory() override
    {
        struct StubPlatformFactory : client::ClientPlatformFactory
        {
            StubPlatformFactory(std::shared_ptr<client::ClientPlatform> const& platform) :
                platform(platform)
            {
            }

            std::shared_ptr<client::ClientPlatform> create_client_platform(client::ClientContext*) override
            {
                return platform;
            }
            std::shared_ptr<client::ClientPlatform> platform;
        };
        return std::make_shared<StubPlatformFactory>(platform);
    }

    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> the_rpc_channel() override
    {
        return channel;
    }
    std::shared_ptr<DummyChannel> channel;
    std::shared_ptr<client::ClientPlatform> platform;
};
}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_CONNECTION_CONFIGURATION_H_ */
