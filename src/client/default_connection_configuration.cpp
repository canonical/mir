/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "default_connection_configuration.h"

#include "rpc/make_rpc_channel.h"
#include "mir_logger.h"
#include "native_client_platform_factory.h"
#include "mir/input/input_platform.h"
#include "mir/default_configuration.h"
#include "logging/rpc_report.h"

namespace mcl = mir::client;

mcl::DefaultConnectionConfiguration::DefaultConnectionConfiguration(
    std::string const& socket_file)
    : user_socket_file{socket_file}
{
}

std::shared_ptr<mcl::rpc::MirBasicRpcChannel>
mcl::DefaultConnectionConfiguration::the_rpc_channel()
{
    return rpc_channel(
        [this]
        {
            return mcl::rpc::make_rpc_channel(the_socket_file(), the_rpc_report());
        });
}

std::shared_ptr<mcl::Logger>
mcl::DefaultConnectionConfiguration::the_logger()
{
    return logger(
        []
        {
            return std::make_shared<mcl::ConsoleLogger>();
        });
}

std::shared_ptr<mcl::ClientPlatformFactory>
mcl::DefaultConnectionConfiguration::the_client_platform_factory()
{
    return client_platform_factory(
        []
        {
            return std::make_shared<mcl::NativeClientPlatformFactory>();
        });
}

std::shared_ptr<mir::input::receiver::InputPlatform>
mcl::DefaultConnectionConfiguration::the_input_platform()
{
    return input_platform(
        []
        {
            return mir::input::receiver::InputPlatform::create();
        });
}

std::string
mcl::DefaultConnectionConfiguration::the_socket_file()
{
    if (user_socket_file.empty())
        return mir::default_server_socket;
    else
        return user_socket_file;
}

std::shared_ptr<mcl::rpc::RpcReport>
mcl::DefaultConnectionConfiguration::the_rpc_report()
{
    return rpc_report(
        [this]
        {
            return std::make_shared<mcl::logging::RpcReport>(the_logger());
        });
}
