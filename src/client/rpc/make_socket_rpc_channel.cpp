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

namespace mcl = mir::client;
namespace mclr = mir::client::rpc;

std::shared_ptr<mclr::MirBasicRpcChannel>
mclr::make_rpc_channel(std::string const& name,
                       std::shared_ptr<mcl::SurfaceMap> const& map,
                       std::shared_ptr<RpcReport> const& rpc_report)
{ 
    return std::make_shared<MirSocketRpcChannel>(name, map, rpc_report);
}
