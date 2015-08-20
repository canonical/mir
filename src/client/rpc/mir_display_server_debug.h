/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#ifndef MIR_CLIENT_RPC_DISPLAY_SERVER_DEBUG_H_
#define MIR_CLIENT_RPC_DISPLAY_SERVER_DEBUG_H_

#include <mir/protobuf/display_server_debug.h>
#include <memory>

namespace mir
{
namespace client
{
namespace rpc
{
class MirBasicRpcChannel;
class DisplayServerDebug : public mir::protobuf::DisplayServerDebug
{
public:
    DisplayServerDebug(std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> const& channel);
    void translate_surface_to_screen(
        mir::protobuf::CoordinateTranslationRequest const* request,
        mir::protobuf::CoordinateTranslationResponse* response,
        google::protobuf::Closure* done) override;

private:
    std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> const channel;
};
}
}
}

#endif //MIR_CLIENT_RPC_DISPLAY_SERVER_DEBUG_H_
