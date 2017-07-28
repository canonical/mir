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

#ifndef MIR_CLIENT_MIR_BUFFER_FACTORY_H
#define MIR_CLIENT_MIR_BUFFER_FACTORY_H

#include "mir/geometry/size.h"
#include "mir/optional_value.h"
#include "mir_protobuf.pb.h"
#include "buffer.h"
#include <mutex>
#include <memory>

namespace mir
{
namespace client
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
class ClientBufferFactory;
class AsyncBufferFactory
{
public:
    virtual ~AsyncBufferFactory() = default;
    AsyncBufferFactory() = default;

    virtual std::unique_ptr<MirBuffer> generate_buffer(mir::protobuf::Buffer const& buffer) = 0;
    virtual void expect_buffer(
        std::shared_ptr<ClientBufferFactory> const& native_buffer_factory,
        MirConnection* connection,
        geometry::Size size,
        MirPixelFormat format,
        MirBufferUsage usage,
        MirBufferCallback cb,
        void* cb_context) = 0;
    virtual void expect_buffer(
        std::shared_ptr<ClientBufferFactory> const& native_buffer_factory,
        MirConnection* connection,
        geometry::Size size,
        uint32_t native_format,
        uint32_t native_flags,
        MirBufferCallback cb,
        void* cb_context) = 0;
    virtual void cancel_requests_with_context(void*) = 0;

private:
    AsyncBufferFactory(AsyncBufferFactory const&) = delete;
    AsyncBufferFactory& operator=(AsyncBufferFactory const&) = delete;
};

class BufferFactory : public AsyncBufferFactory
{
public:
    std::unique_ptr<MirBuffer> generate_buffer(mir::protobuf::Buffer const& buffer) override;
    void expect_buffer(
        std::shared_ptr<ClientBufferFactory> const& native_buffer_factory,
        MirConnection* connection,
        geometry::Size size,
        MirPixelFormat format,
        MirBufferUsage usage,
        MirBufferCallback cb,
        void* cb_context) override;
    void expect_buffer(
        std::shared_ptr<ClientBufferFactory> const& native_buffer_factory,
        MirConnection* connection,
        geometry::Size size,
        uint32_t native_format,
        uint32_t native_flags,
        MirBufferCallback cb,
        void* cb_context) override;
    void cancel_requests_with_context(void*) override;

private:
    std::mutex mutex;
    int error_id { -1 };

    struct AllocationRequest
    {
        struct NativeRequest
        {
            uint32_t native_format;
            uint32_t native_flags;
        };

        struct SoftwareRequest
        {
            MirPixelFormat format;
            MirBufferUsage usage;
        };

        AllocationRequest(
            std::shared_ptr<ClientBufferFactory> const& native_buffer_factory,
            MirConnection* connection,
            geometry::Size size,
            MirPixelFormat format,
            MirBufferUsage usage,
            MirBufferCallback cb,
            void* cb_context);
        AllocationRequest(
            std::shared_ptr<ClientBufferFactory> const& native_buffer_factory,
            MirConnection* connection,
            geometry::Size size,
            uint32_t native_format,
            uint32_t native_flags,
            MirBufferCallback cb,
            void* cb_context);

        std::shared_ptr<ClientBufferFactory> const native_buffer_factory;
        MirConnection* connection;
        geometry::Size size;
        optional_value<NativeRequest> native_request;
        optional_value<SoftwareRequest> sw_request;
        MirBufferCallback cb;
        void* cb_context;
    };
    std::vector<std::unique_ptr<AllocationRequest>> allocation_requests;
};
#pragma GCC diagnostic pop
}
}
#endif /* MIR_CLIENT_MIR_BUFFER_FACTORY_H_ */
