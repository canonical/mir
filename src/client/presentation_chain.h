/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_CLIENT_PRESENTATION_CHAIN_H
#define MIR_CLIENT_PRESENTATION_CHAIN_H

#include "mir_presentation_chain.h"
#include "mir/geometry/size.h"
#include "mir_toolkit/mir_presentation_chain.h"
#include "mir_protobuf.pb.h"
#include "buffer.h"
#include <mutex>
#include <memory>

namespace mir
{
namespace client
{
class ClientBufferFactory;
class ClientBuffer;
namespace rpc
{
class DisplayServer;
}

struct AsyncBufferFactory
{
    std::unique_ptr<Buffer> generate_buffer(mir::protobuf::Buffer const& buffer);
    void expect_buffer(
        std::shared_ptr<ClientBufferFactory> const& factory,
        geometry::Size size,
        MirPixelFormat format,
        MirBufferUsage usage,
        mir_buffer_callback cb,
        void* cb_context);

    std::mutex mutex;
    struct AllocationRequest
    {
        AllocationRequest(
            std::shared_ptr<ClientBufferFactory> const& factory,
            geometry::Size size,
            MirPixelFormat format,
            MirBufferUsage usage,
            mir_buffer_callback cb,
            void* cb_context);

        std::shared_ptr<ClientBufferFactory> const factory;
        geometry::Size size;
        MirPixelFormat format;
        MirBufferUsage usage;
        mir_buffer_callback cb;
        void* cb_context;
    };
    std::vector<std::unique_ptr<AllocationRequest>> allocation_requests;
};
class PresentationChain : public MirPresentationChain
{
public:
    PresentationChain(
        MirConnection* connection,
        int rpc_id,
        rpc::DisplayServer& server,
        std::shared_ptr<ClientBufferFactory> const& cbfactory,
        std::shared_ptr<AsyncBufferFactory> const& factory);
    void allocate_buffer(
        geometry::Size size, MirPixelFormat format, MirBufferUsage usage,
        mir_buffer_callback callback, void* context) override;
    void submit_buffer(MirBuffer* buffer) override;
    void release_buffer(MirBuffer* buffer) override;

    void buffer_available(mir::protobuf::Buffer const& buffer) override;
    void buffer_unavailable() override;

    MirConnection* connection() const override;
    int rpc_id() const override;
    char const* error_msg() const override;
private:

    MirConnection* const connection_;
    int const stream_id;
    rpc::DisplayServer& server;
    std::shared_ptr<ClientBufferFactory> const cfactory;
    std::shared_ptr<AsyncBufferFactory> const factory;

    std::mutex mutex;
    std::vector<std::unique_ptr<Buffer>> buffers;
};
}
}
#endif /* MIR_CLIENT_PRESENTATION_CHAIN_H */
