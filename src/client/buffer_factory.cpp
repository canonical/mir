/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/client_buffer_factory.h"
#include "buffer_factory.h"
#include <algorithm>
#include <boost/throw_exception.hpp>
#include "protobuf_to_native_buffer.h"

namespace mcl = mir::client;
namespace geom = mir::geometry;

mcl::BufferFactory::AllocationRequest::AllocationRequest(
    std::shared_ptr<mcl::ClientBufferFactory> const& native_buffer_factory,
    MirPresentationChain* chain,
    geom::Size size, MirPixelFormat format, MirBufferUsage usage,
    mir_buffer_callback cb, void* cb_context) :
    native_buffer_factory(native_buffer_factory),
    chain(chain),
    size(size),
    format(format),
    usage(usage),
    cb(cb),
    cb_context(cb_context)
{
}

void mcl::BufferFactory::expect_buffer(
    std::shared_ptr<mcl::ClientBufferFactory> const& factory,
    MirPresentationChain* chain,
    geometry::Size size,
    MirPixelFormat format,
    MirBufferUsage usage,
    mir_buffer_callback cb,
    void* cb_context)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    allocation_requests.emplace_back(
        std::make_unique<AllocationRequest>(factory, chain, size, format, usage, cb, cb_context));
}

std::unique_ptr<mcl::Buffer> mcl::BufferFactory::generate_buffer(mir::protobuf::Buffer const& buffer)
{
    std::lock_guard<decltype(mutex)> lk(mutex);
    //must be new, allocate and send it.
    auto request_it = std::find_if(allocation_requests.begin(), allocation_requests.end(),
        [&buffer](std::unique_ptr<AllocationRequest> const& it)
        {
            return geom::Size{buffer.width(), buffer.height()} == it->size;
        });

    if (request_it == allocation_requests.end())
        BOOST_THROW_EXCEPTION(std::logic_error("unrequested buffer received"));

    auto b = std::make_unique<Buffer>(
        (*request_it)->cb, (*request_it)->cb_context,
        buffer.buffer_id(),
        (*request_it)->native_buffer_factory->create_buffer(
            mcl::protobuf_to_native_buffer(buffer),
            (*request_it)->size, (*request_it)->format),
            (*request_it)->chain,
            (*request_it)->usage);

    allocation_requests.erase(request_it);
    return std::move(b);
}
