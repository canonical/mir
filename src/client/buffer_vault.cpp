/*
 * Copyright © 2015 Canonical Ltd.
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
#include "mir/client/client_buffer.h"
#include "mir/client/surface_map.h"
#include "buffer_vault.h"
#include "buffer.h"
#include "buffer_factory.h"
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
void ignore_buffer(MirBuffer*, void*)
{
}
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
    buffer_factory->cancel_requests_with_context(this);

    std::vector<std::shared_ptr<MirBuffer>> current_buffers;

    std::unique_lock<std::mutex> lk(mutex);

    // Prevent callbacks from allocating new buffers
    being_destroyed = true;

    for (auto& it : buffers)
    if (auto map = surface_map.lock())
    {
        if (auto buffer = map->buffer(it.first))
        {
            /*
             * Annoying wart:
             *
             * mir::AtomicCallback is atomic via locking, and we need it to be because
             * we rely on the guarantee that after set_callback() returns no thread
             * is in, or will enter, the previous callback.
             *
             * However!
             *
             * wire_transfer_inbound() is called from buffer->callback, and acquires
             * BufferVault::mutex.
             *
             * This destuctor must call buffer->set_callback(), and has acquired
             * BufferVault::mutex.
             *
             * So we've got a lock inversion.
             *
             * We can't unlock here, as wire_transfer_inbound() might mutate
             * BufferVault::buffers, which we're iterating over.
             *
             * So store them up to process later...
             */
            current_buffers.push_back(buffer);

            /*
             * ...and erase them from the SurfaceMap.
             *
             * After this point, the RPC layer will ignore any messages about
             * this buffer.
             *
             * We don't need to explicitly ask the server to free it; it'll be
             * freed with the BufferStream
             */
            map->erase(it.first);
        }
    }
    lk.unlock();

    /*
     * Now, ensure that no threads are in the existing buffer callbacks...
     */
    for (auto const& buffer : current_buffers)
    {
        buffer->set_callback(ignore_buffer, nullptr);
    }
}

void mcl::BufferVault::alloc_buffer(geom::Size size, MirPixelFormat format, int usage)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    buffer_factory->expect_buffer(platform_factory, nullptr, size, format, static_cast<MirBufferUsage>(usage),
        incoming_buffer, this);
#pragma GCC diagnostic pop
    server_requests->allocate_buffer(size, format, usage);
}

void mcl::BufferVault::free_buffer(int free_id)
{
    server_requests->free_buffer(free_id);
    if (auto map = surface_map.lock())
    {
        map->erase(free_id);
    }
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

mcl::BufferVault::BufferMap::iterator mcl::BufferVault::available_buffer()
{
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
    return it;
}

mcl::NoTLSFuture<std::shared_ptr<mcl::MirBuffer>> mcl::BufferVault::withdraw()
{
    std::vector<int> free_ids;
    std::unique_lock<std::mutex> lk(mutex);
    if (disconnected_)
        BOOST_THROW_EXCEPTION(std::logic_error("server_disconnected"));

    //clean up incorrectly sized buffers
    for (auto it = buffers.begin(); it != buffers.end();)
    {
        auto buffer = checked_buffer_from_map(it->first);
        if ((it->second == Owner::Self) && (buffer->size() != size)) 
        {
            current_buffer_count--;
            free_ids.push_back(it->first);
            it = buffers.erase(it);
        }
        else
        {
            it++;
        }
    } 

    mcl::NoTLSPromise<std::shared_ptr<mcl::MirBuffer>> promise;
    auto it = available_buffer();
    auto future = promise.get_future();
    if (it != buffers.end())
    {
        it->second = Owner::ContentProducer;
        promise.set_value(checked_buffer_from_map(it->first));
        lk.unlock();
    }
    else
    {
        promises.emplace_back(std::move(promise));

        auto s = size;
        bool allocate_buffer = (current_buffer_count <  needed_buffer_count);
        if (allocate_buffer)
            current_buffer_count++;
        lk.unlock();

        if (allocate_buffer)
            alloc_buffer(s, format, usage);
    }

    for(auto& id : free_ids)
        free_buffer(id);
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

MirWaitHandle* mcl::BufferVault::wire_transfer_outbound(
    std::shared_ptr<mcl::MirBuffer> const& buffer, std::function<void()> const& done)
{
    std::unique_lock<std::mutex> lk(mutex);
    auto it = buffers.find(buffer->rpc_id());
    if (it == buffers.end() || it->second != Owner::SelfWithContent)
        BOOST_THROW_EXCEPTION(std::logic_error("buffer cannot be transferred"));
    it->second = Owner::Server;
    lk.unlock();

    buffer->submitted();
    server_requests->submit_buffer(*buffer);

    lk.lock();
    if (disconnected_)
        BOOST_THROW_EXCEPTION(std::logic_error("server_disconnected"));
    if (buffers.end() != available_buffer())
    {
        done();
    }
    else
    {
        swap_buffers_wait_handle.expect_result();
        deferred_cb = done;
    }
    return &swap_buffers_wait_handle;
}

void mcl::BufferVault::wire_transfer_inbound(int buffer_id)
{
    std::unique_lock<std::mutex> lk(mutex);

    /*
     * The BufferVault is in the process of being destroyed; there's no point in
     * doing any processing and we should *certainly* not allocate any more buffers.
     */
    if (being_destroyed)
        return;

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
            if (should_decrease_count)
                current_buffer_count--;
            lk.unlock();

            free_buffer(id);
            if (!should_decrease_count)
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

    trigger_callback(std::move(lk));
}

void mcl::BufferVault::disconnected()
{
    std::unique_lock<std::mutex> lk(mutex);
    disconnected_ = true;
    promises.clear();
    trigger_callback(std::move(lk));
}

void mcl::BufferVault::trigger_callback(std::unique_lock<std::mutex> lk)
{
    if (auto cb = deferred_cb)
    {
        deferred_cb = {};
        lk.unlock();
        cb();
        swap_buffers_wait_handle.result_received();
    }
}

void mcl::BufferVault::set_scale(float scale)
{
    std::unique_lock<std::mutex> lk(mutex);
    set_size(lk, size * scale);
}

void mcl::BufferVault::set_size(geom::Size new_size)
{
    std::unique_lock<std::mutex> lk(mutex);
    set_size(lk, new_size);
}

void mcl::BufferVault::set_size(std::unique_lock<std::mutex> const&, geometry::Size new_size)
{
    size = new_size;
}

void mcl::BufferVault::set_interval(int i)
{
    std::unique_lock<std::mutex> lk(mutex);

    if (i == interval)
        return;
    interval = i;

    if (i == 0)
    {
        current_buffer_count++;
        needed_buffer_count++;
        lk.unlock();
        alloc_buffer(size, format, usage);
    }
    else
    {
        needed_buffer_count = initial_buffer_count;
        while (current_buffer_count > needed_buffer_count)
        {
            auto it = std::find_if(buffers.begin(), buffers.end(),
                [](auto const& entry) { return entry.second == Owner::Self; });
            if (it == buffers.end())
                break;
            current_buffer_count--;
            int id = it->first;
            buffers.erase(it);
            lk.unlock();
            free_buffer(id);
            lk.lock();
        }
    }
}
