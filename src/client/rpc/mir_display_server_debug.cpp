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

#include "mir_display_server_debug.h"
#include "mir_basic_rpc_channel.h"

#include <string>

namespace mclr = mir::client::rpc;

mclr::DisplayServerDebug::DisplayServerDebug(std::shared_ptr<mir::client::rpc::MirBasicRpcChannel> const& channel)
    : channel{channel}
{
}

void mclr::DisplayServerDebug::translate_surface_to_screen(
    mir::protobuf::CoordinateTranslationRequest const* request,
    mir::protobuf::CoordinateTranslationResponse* response,
    google::protobuf::Closure* done)
{
    channel->call_method(std::string(__func__), request, response, done);
}
