/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 2 or 3 as
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_PRESENTATION_CHAIN_H
#define MIR_CLIENT_PRESENTATION_CHAIN_H

#include "mir_presentation_chain.h"
#include "buffer_stream.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/mir_presentation_chain.h"
#include "mir_protobuf.pb.h"
#include "buffer.h"

#include <memory>

namespace mir
{
namespace client
{
class ClientBufferFactory;
class ClientBuffer;
class AsyncBufferFactory;
namespace rpc
{
class DisplayServer;
}

class PresentationChain : public MirPresentationChain
{
public:
    PresentationChain(
        MirConnection* connection,
        int rpc_id,
        rpc::DisplayServer& server,
        std::shared_ptr<ClientBufferFactory> const& native_buffer_factory,
        std::shared_ptr<AsyncBufferFactory> const& mir_buffer_factory);
    void submit_buffer(MirBuffer* buffer) override;
    MirConnection* connection() const override;
    int rpc_id() const override;
    char const* error_msg() const override;
    void set_dropping_mode() override;
    void set_queueing_mode() override;

private:

    MirConnection* const connection_;
    int const stream_id;
    rpc::DisplayServer& server;
    std::shared_ptr<ClientBufferFactory> const native_buffer_factory;
    std::shared_ptr<AsyncBufferFactory> const mir_buffer_factory;

    BufferStreamConfiguration interval_config;
};
}
}
#endif /* MIR_CLIENT_PRESENTATION_CHAIN_H */
