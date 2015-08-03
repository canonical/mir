/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_CLIENT_PRIVATE_H_
#define MIR_CLIENT_PRIVATE_H_

#include <mir_toolkit/client_types.h>

#include <memory>

namespace mir
{
namespace client
{
namespace rpc
{
class MirBasicRpcChannel;
}
/**
 * Get the the RpcChannel associated with the connection.
 * This is a "private" function to support development of client-side protobuf RPC calls
 * using the Mir infrastructure. This intended for internal prototyping and there is no
 * commitment to long term support.
 *
 * @param connection - a connection to a Mir server
 * @return the RpcChannel associated with the connection
 */
std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> the_rpc_channel(MirConnection* connection);
}
}

#endif /* MIR_CLIENT_PRIVATE_H_ */
