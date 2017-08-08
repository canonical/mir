/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/client/client_buffer_factory.h"
#include "buffer_factory.h"
#include "error_buffer.h"
#include <algorithm>
#include <boost/throw_exception.hpp>
#include "protobuf_to_native_buffer.h"

namespace mcl = mir::client;
namespace geom = mir::geometry;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
mcl::BufferFactory::AllocationRequest::AllocationRequest(
    std::shared_ptr<mcl::ClientBufferFactory> const& native_buffer_factory,
    MirConnection* connection,
    geom::Size size, MirPixelFormat format, MirBufferUsage usage,
    MirBufferCallback cb, void* cb_context) :
    native_buffer_factory(native_buffer_factory),
    connection(connection),
    size(size),
    sw_request(SoftwareRequest{format, usage}),
    cb(cb),
    cb_context(cb_context)
{
}

mcl::BufferFactory::AllocationRequest::AllocationRequest(
    std::shared_ptr<mcl::ClientBufferFactory> const& native_buffer_factory,
    MirConnection* connection,
    geom::Size size, uint32_t native_format, uint32_t native_flags,
    MirBufferCallback cb, void* cb_context) :
    native_buffer_factory(native_buffer_factory),
    connection(connection),
    size(size),
    native_request(NativeRequest{native_format, native_flags}),
    cb(cb),
    cb_context(cb_context)
{
}

void mcl::BufferFactory::expect_buffer(
    std::shared_ptr<mcl::ClientBufferFactory> const& factory,
    MirConnection* connection,
    geometry::Size size,
    MirPixelFormat format,
    MirBufferUsage usage,
    MirBufferCallback cb,
    void* cb_context)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    allocation_requests.emplace_back(
        std::make_unique<AllocationRequest>(factory, connection, size, format, usage, cb, cb_context));
}
#pragma GCC diagnostic pop

void mcl::BufferFactory::expect_buffer(
    std::shared_ptr<mcl::ClientBufferFactory> const& factory,
    MirConnection* connection,
    geometry::Size size,
    uint32_t native_format,
    uint32_t native_flags,
    MirBufferCallback cb,
    void* cb_context)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    allocation_requests.emplace_back(
        std::make_unique<AllocationRequest>(factory, connection, size, native_format, native_flags, cb, cb_context));
}

std::unique_ptr<mcl::MirBuffer> mcl::BufferFactory::generate_buffer(mir::protobuf::Buffer const& buffer)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    auto request_it = std::find_if(allocation_requests.begin(), allocation_requests.end(),
        [&buffer](std::unique_ptr<AllocationRequest> const& it)
        {
            return geom::Size{buffer.width(), buffer.height()} == it->size;
        });

    if (request_it == allocation_requests.end())
        BOOST_THROW_EXCEPTION(std::logic_error("unrequested buffer received"));

    std::unique_ptr<mcl::MirBuffer> b;
    if (buffer.has_error())
    {
        b = std::make_unique<ErrorBuffer>(
            buffer.error(), error_id--, 
            (*request_it)->cb, (*request_it)->cb_context, (*request_it)->connection);
    }
    else
    {
        auto factory = (*request_it)->native_buffer_factory;
        std::shared_ptr<mcl::ClientBuffer> client_buffer;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        auto usage = mir_buffer_usage_hardware;
#pragma GCC diagnostic pop
        if ((*request_it)->native_request.is_set())
        {
            auto& req = (*request_it)->native_request.value();
            client_buffer = factory->create_buffer(
                mcl::protobuf_to_native_buffer(buffer), req.native_format, req.native_flags);
        }
        else if ((*request_it)->sw_request.is_set())
        {
            auto& req = (*request_it)->sw_request.value();
            usage = req.usage;
            client_buffer = factory->create_buffer(
                mcl::protobuf_to_native_buffer(buffer), (*request_it)->size, req.format);
        }
        else
        {
            BOOST_THROW_EXCEPTION(std::logic_error("could not create buffer"));
        }

        b = std::make_unique<Buffer>(
            (*request_it)->cb, (*request_it)->cb_context, buffer.buffer_id(),
            client_buffer, (*request_it)->connection, usage);
    }

    allocation_requests.erase(request_it);
    return b;
}

void mcl::BufferFactory::cancel_requests_with_context(void* cancelled_context)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    auto it = allocation_requests.begin();
    while (it != allocation_requests.end())
    {
        if ((*it)->cb_context == cancelled_context)
            it = allocation_requests.erase(it);
        else
            it++;
    }
}
