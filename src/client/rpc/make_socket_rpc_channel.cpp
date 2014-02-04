/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "make_rpc_channel.h"
#include "mir_socket_rpc_channel.h"

#include <cstring>

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;

namespace
{
struct Prefix
{
    template<int Size>
    Prefix(char const (&prefix)[Size]) : size(Size-1), prefix(prefix) {}

    bool is_start_of(std::string const& name) const
    { return !strncmp(name.c_str(), prefix, size); }

    int const size;
    char const* const prefix;
} const fd_prefix("fd://");
}

std::shared_ptr<google::protobuf::RpcChannel>
mclr::make_rpc_channel(std::string const& name,
                       std::shared_ptr<mcl::SurfaceMap> const& map,
                       std::shared_ptr<mcl::DisplayConfiguration> const& disp_conf,
                       std::shared_ptr<RpcReport> const& rpc_report,
                       std::shared_ptr<mcl::LifecycleControl> const& lifecycle_control)
{
    if (fd_prefix.is_start_of(name))
    {
        auto const fd = atoi(name.c_str()+fd_prefix.size);
        return std::make_shared<MirSocketRpcChannel>(fd, map, disp_conf, rpc_report, lifecycle_control);
    }

    return std::make_shared<MirSocketRpcChannel>(name, map, disp_conf, rpc_report, lifecycle_control);
}
