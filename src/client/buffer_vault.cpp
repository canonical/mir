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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/client_buffer_factory.h"
#include "mir/client_buffer.h"
#include "buffer_vault.h"
#include "buffer.h"
#include "buffer_factory.h"
#include "surface_map.h"
#include "mir_protobuf.pb.h"
#include "protobuf_to_native_buffer.h"
#include "connection_surface_map.h"
#include "buffer_factory.h"
#include "buffer.h"
#include <algorithm>
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;

enum class mcl::BufferVault::Owner
{
    Server,
    Self,
    ContentProducer,
    SelfWithContent
};

namespace
{
void incoming_buffer(MirBuffer* buffer, void* context)
{
    auto vault = static_cast<mcl::BufferVault*>(context);
    vault->wire_transfer_inbound(reinterpret_cast<mcl::Buffer*>(buffer)->rpc_id());
}
}

mcl::BufferVault::BufferVault(
    std::shared_ptr<ClientBufferFactory> const& platform_factory,
    std::shared_ptr<AsyncBufferFactory> const& buffer_factory,
    std::shared_ptr<ServerBufferRequests> const& server_requests,
    std::weak_ptr<SurfaceMap> const& surface_map,
    geom::Size size, MirPixelFormat format, int usage, unsigned int initial_nbuffers) :
    platform_factory(platform_factory),
    buffer_factory(buffer_factory),
    server_requests(server_requests),
    surface_map(surface_map),
    format(format),
    usage(usage),
    size(size),
    disconnected_(false),
    current_buffer_count(initial_nbuffers),
    needed_buffer_count(initial_nbuffers),
    initial_buffer_count(initial_nbuffers)
{
    for (auto i = 0u; i < initial_buffer_count; i++)
        alloc_buffer(size, format, usage);
}

mcl::BufferVault::~BufferVault()
{
    if (disconnected_)
        return;

    buffer_factory->cancel_requests_with_context(this);
    for (auto& it : buffers)
    try
    {
        free_buffer(it.first);
    }
    catch (...)
    {
    }
}

void mcl::BufferVault::alloc_buffer(geom::Size size, MirPixelFormat format, int usage)
{
    buffer_factory->expect_buffer(platform_factory, nullptr, size, format, static_cast<MirBufferUsage>(usage),
        incoming_buffer, this);
    server_requests->allocate_buffer(size, format, usage);
}

void mcl::BufferVault::free_buffer(int free_id)
{
    server_requests->free_buffer(free_id);
}

void mcl::BufferVault::realloc_buffer(int free_id, geom::Size size, MirPixelFormat format, int usage)
{
    free_buffer(free_id);
    alloc_buffer(size, format, usage);
}

std::shared_ptr<mcl::MirBuffer> mcl::BufferVault::checked_buffer_from_map(int id)
{
    auto map = surface_map.lock();
    if (!map)
        BOOST_THROW_EXCEPTION(std::logic_error("connection resources lost; cannot access buffer"));
    
    if (auto buffer = map->buffer(id))
        return buffer;
    else
        BOOST_THROW_EXCEPTION(std::logic_error("no buffer in map"));
}

mcl::NoTLSFuture<std::shared_ptr<mcl::MirBuffer>> mcl::BufferVault::withdraw()
{
    std::lock_guard<std::mutex> lk(mutex);
    if (disconnected_)
        BOOST_THROW_EXCEPTION(std::logic_error("server_disconnected"));
    mcl::NoTLSPromise<std::shared_ptr<mcl::MirBuffer>> promise;
    auto it = std::find_if(buffers.begin(), buffers.end(),
        [this](std::pair<int, Owner> const& entry) {
            return ((entry.second == Owner::Self) &&
                    (checked_buffer_from_map(entry.first)->size() == size) &&
                    (entry.first != last_received_id)); });
    if (it == buffers.end())
        it = std::find_if(buffers.begin(), buffers.end(),
        [this](std::pair<int, Owner> const& entry) {
            return ((entry.second == Owner::Self) &&
                    (checked_buffer_from_map(entry.first)->size() == size)); });

    auto future = promise.get_future();
    if (it != buffers.end())
    {
        it->second = Owner::ContentProducer;
        promise.set_value(checked_buffer_from_map(it->first));
    }
    else
    {
        promises.emplace_back(std::move(promise));
    }
    return future;
}

void mcl::BufferVault::deposit(std::shared_ptr<mcl::MirBuffer> const& buffer)
{
    std::lock_guard<std::mutex> lk(mutex);
    auto it = buffers.find(buffer->rpc_id());
    if (it == buffers.end() || it->second != Owner::ContentProducer)
        BOOST_THROW_EXCEPTION(std::logic_error("buffer cannot be deposited"));

    it->second = Owner::SelfWithContent;
    checked_buffer_from_map(it->first)->increment_age();
}

void mcl::BufferVault::wire_transfer_outbound(std::shared_ptr<mcl::MirBuffer> const& buffer)
{
    std::unique_lock<std::mutex> lk(mutex);
    auto it = buffers.find(buffer->rpc_id());
    if (it == buffers.end() || it->second != Owner::SelfWithContent)
        BOOST_THROW_EXCEPTION(std::logic_error("buffer cannot be transferred"));
    it->second = Owner::Server;
    lk.unlock();

    buffer->submitted();
    server_requests->submit_buffer(*buffer);
}

void mcl::BufferVault::wire_transfer_inbound(int buffer_id)
{
    std::unique_lock<std::mutex> lk(mutex);
    last_received_id = buffer_id;
    auto buffer = checked_buffer_from_map(buffer_id);
    auto inbound_size = buffer->size();
    auto it = buffers.find(buffer_id);
    if (it == buffers.end())
    {
        if (inbound_size != size)
        {
            lk.unlock();
            realloc_buffer(buffer_id, size, format, usage);
            return;
        }
        buffers[buffer_id] = Owner::Self;
    }
    else
    {
        auto should_decrease_count = (current_buffer_count > needed_buffer_count);
        if (size != buffer->size() || should_decrease_count)
        {
            auto id = it->first;
            buffers.erase(it);
            lk.unlock();

            free_buffer(id);
            if (should_decrease_count)
                current_buffer_count--;
            else
                alloc_buffer(size, format, usage);
            return;
        }
        else
        {
            it->second = Owner::Self;
        }
    }

    if (!promises.empty())
    {
        buffers[buffer_id] = Owner::ContentProducer;
        promises.front().set_value(buffer);
        promises.pop_front();
    }
}

//TODO: the server will currently spam us with a lot of resize messages at once,
//      and we want to delay the IPC transactions for resize. If we could rate-limit
//      the incoming messages, we should should consolidate the scale and size functions
void mcl::BufferVault::set_size(geom::Size sz)
{
    std::lock_guard<std::mutex> lk(mutex);
    size = sz;
}

void mcl::BufferVault::disconnected()
{
    std::lock_guard<std::mutex> lk(mutex);
    disconnected_ = true;
    promises.clear();
}

void mcl::BufferVault::set_scale(float scale)
{
    std::vector<int> free_ids;
    std::unique_lock<std::mutex> lk(mutex);
    auto new_size = size * scale;
    if (new_size == size)
        return;
    size = new_size;
    for (auto it = buffers.begin(); it != buffers.end();)
    {
        auto buffer = checked_buffer_from_map(it->first);
        if ((it->second == Owner::Self) && (buffer->size() != size)) 
        {
            free_ids.push_back(it->first);
            it = buffers.erase(it);
        }
        else
        {
            it++;
        }
    } 
    lk.unlock();

    for(auto& id : free_ids)
        realloc_buffer(id, new_size, format, usage);
}

void mcl::BufferVault::increase_buffer_count()
{
    std::unique_lock<std::mutex> lk(mutex);
    current_buffer_count++;
    needed_buffer_count++;
    lk.unlock();
    
    alloc_buffer(size, format, usage);
}

void mcl::BufferVault::decrease_buffer_count()
{
    std::unique_lock<std::mutex> lk(mutex);
    if (current_buffer_count == initial_buffer_count)
        return;
    needed_buffer_count--;

    auto it = std::find_if(buffers.begin(), buffers.end(),
        [](auto const& entry) { return entry.second == Owner::Self; });
    if (it != buffers.end())
    {
        current_buffer_count--;
        int free_id = it->first;
        buffers.erase(it);
        lk.unlock();
        free_buffer(free_id);
    }
}
