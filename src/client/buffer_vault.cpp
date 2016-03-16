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
    ContentProducer,
    Self
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
    std::shared_ptr<ClientBufferFactory> const& client_buffer_factory,
    std::shared_ptr<ServerBufferRequests> const& server_requests,
    std::shared_ptr<SurfaceMap> const& map,
    std::shared_ptr<AsyncBufferFactory> const& mirfactory,
    geom::Size size, MirPixelFormat format, int usage, unsigned int initial_nbuffers) :
    factory(client_buffer_factory),
    server_requests(server_requests),
    map(map), mirfactory(mirfactory),
    format(format),
    usage(usage),
    size(size),
    disconnected_(false)
{
    for (auto i = 0u; i < initial_nbuffers; i++)
    {
        mirfactory->expect_buffer(factory, nullptr, size, format, (MirBufferUsage)usage, incoming_buffer, nullptr);
        server_requests->allocate_buffer(size, format, usage);
    }
}

mcl::BufferVault::~BufferVault()
{
    if (disconnected_)
        return;

    for (auto& it : buffers)
    try
    {
        map->erase(it.first);
        server_requests->free_buffer(it.first);
    }
    catch (...)
    {
    }
}


void mcl::BufferVault::realloc(int free_id, geom::Size, MirPixelFormat format, int usage)
{
    map->erase(free_id);
    server_requests->free_buffer(free_id);

    mirfactory->expect_buffer(factory, nullptr, size, format, (MirBufferUsage)usage,
        incoming_buffer, this);
    server_requests->allocate_buffer(size, format, usage);
}

mcl::NoTLSFuture<std::shared_ptr<mcl::Buffer>> mcl::BufferVault::withdraw()
{
    std::lock_guard<std::mutex> lk(mutex);
    if (disconnected_)
        BOOST_THROW_EXCEPTION(std::logic_error("server_disconnected"));
    mcl::NoTLSPromise<std::shared_ptr<mcl::Buffer>> promise;
    auto it = std::find_if(buffers.begin(), buffers.end(),
        [this](std::pair<int, BufferEntry> const& entry) { 
            return ((entry.second.owner == Owner::Self) && (size == entry.second.buffer->size())); });

    auto future = promise.get_future();
    if (it != buffers.end())
    {
        it->second.owner = Owner::ContentProducer;
        promise.set_value(it->second.buffer);
    }
    else
    {
        promises.emplace_back(std::move(promise));
    }
    return future;
}

void mcl::BufferVault::deposit(std::shared_ptr<mcl::Buffer> const& buffer)
{
    std::lock_guard<std::mutex> lk(mutex);
    auto it = std::find_if(buffers.begin(), buffers.end(),
        [&buffer](std::pair<int, BufferEntry> const& entry) { return buffer == entry.second.buffer; });
    if (it == buffers.end() || it->second.owner != Owner::ContentProducer)
        BOOST_THROW_EXCEPTION(std::logic_error("buffer cannot be deposited"));

    it->second.owner = Owner::Self;
    it->second.buffer->client_buffer()->increment_age();
}

void mcl::BufferVault::wire_transfer_outbound(std::shared_ptr<mcl::Buffer> const& buffer)
{
    int id;
    std::shared_ptr<mcl::ClientBuffer> submit_buffer;
    std::unique_lock<std::mutex> lk(mutex);
    auto it = std::find_if(buffers.begin(), buffers.end(),
        [&buffer](std::pair<int, BufferEntry> const& entry) { return buffer == entry.second.buffer; });
    if (it == buffers.end() || it->second.owner != Owner::Self)
        BOOST_THROW_EXCEPTION(std::logic_error("buffer cannot be transferred"));

    it->second.owner = Owner::Server;
    it->second.buffer->submitted();
    it->second.buffer->client_buffer()->mark_as_submitted();
    submit_buffer = it->second.buffer->client_buffer();
    id = it->first;
    lk.unlock();
    server_requests->submit_buffer(id, *submit_buffer);
}

void mcl::BufferVault::wire_transfer_inbound(int buffer_id)
{
    std::unique_lock<std::mutex> lk(mutex);

    auto buffer = map->buffer(buffer_id);
    auto inbound_size = buffer->size();
    auto it = buffers.find(buffer_id);
    if (it == buffers.end())
    {
        if (inbound_size != size)
        {
            lk.unlock();
            realloc(buffer_id, size, format, usage);
            return;
        }

        buffers[buffer_id] = BufferEntry{ buffer, Owner::Self };
    }
    else
    {
        if (size == inbound_size)
        { 
            it->second.owner = Owner::Self;
        }
        else
        {
            buffers.erase(it);
            lk.unlock();
            realloc(buffer_id, size, format, usage);
            return;
        }
    }

    if (!promises.empty())
    {
        buffers[buffer_id].owner = Owner::ContentProducer;
        promises.front().set_value(buffers[buffer_id].buffer);
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
        if ((it->second.owner == Owner::Self) && (it->second.buffer->size() != size)) 
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
    {
        realloc(id, new_size, format, usage);
    }
}
