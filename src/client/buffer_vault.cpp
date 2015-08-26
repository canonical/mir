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

mcl::BufferVault::BufferVault(
    std::shared_ptr<ClientBufferFactory> const& client_buffer_factory,
    std::shared_ptr<ServerBufferRequests> const& server_requests,
    geom::Size size, MirPixelFormat format, int usage, unsigned int initial_nbuffers) :
    factory(client_buffer_factory),
    server_requests(server_requests),
    format(format),
    usage(usage),
    size(size)
{
    for (auto i = 0u; i < initial_nbuffers; i++)
        server_requests->allocate_buffer(size, format, usage);
}

mcl::BufferVault::~BufferVault()
{
    for (auto& it : buffers)
        server_requests->free_buffer(it.first);
}

std::future<std::shared_ptr<mcl::ClientBuffer>> mcl::BufferVault::withdraw()
{
    std::lock_guard<std::mutex> lk(mutex);
    std::promise<std::shared_ptr<mcl::ClientBuffer>> promise;
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

void mcl::BufferVault::deposit(std::shared_ptr<mcl::ClientBuffer> const& buffer)
{
    std::lock_guard<std::mutex> lk(mutex);
    auto it = std::find_if(buffers.begin(), buffers.end(),
        [&buffer](std::pair<int, BufferEntry> const& entry) { return buffer == entry.second.buffer; });
    if (it == buffers.end() || it->second.owner != Owner::ContentProducer)
        BOOST_THROW_EXCEPTION(std::logic_error("buffer cannot be deposited"));

    it->second.owner = Owner::Self;
    it->second.buffer->increment_age();
}

void mcl::BufferVault::wire_transfer_outbound(std::shared_ptr<mcl::ClientBuffer> const& buffer)
{
    std::lock_guard<std::mutex> lk(mutex);
    auto it = std::find_if(buffers.begin(), buffers.end(),
        [&buffer](std::pair<int, BufferEntry> const& entry) { return buffer == entry.second.buffer; });
    if (it == buffers.end() || it->second.owner != Owner::Self)
        BOOST_THROW_EXCEPTION(std::logic_error("buffer cannot be transferred"));

    it->second.owner = Owner::Server;
    it->second.buffer->mark_as_submitted();
    server_requests->submit_buffer(*it->second.buffer);
}

void mcl::BufferVault::wire_transfer_inbound(mp::Buffer const& protobuf_buffer)
{
    auto package = std::make_shared<MirBufferPackage>();
    package->data_items = protobuf_buffer.data_size();
    package->fd_items = protobuf_buffer.fd_size();
    for (int i = 0; i != protobuf_buffer.data_size(); ++i)
        package->data[i] = protobuf_buffer.data(i);
    for (int i = 0; i != protobuf_buffer.fd_size(); ++i)
        package->fd[i] = protobuf_buffer.fd(i);
    package->stride = protobuf_buffer.stride();
    package->flags = protobuf_buffer.flags();
    package->width = protobuf_buffer.width();
    package->height = protobuf_buffer.height();

    std::unique_lock<std::mutex> lk(mutex);
    auto it = buffers.find(protobuf_buffer.buffer_id());
    if (it == buffers.end())
    {
        auto buffer = factory->create_buffer(package, geom::Size{package->width, package->height}, format);
        buffers[protobuf_buffer.buffer_id()] = BufferEntry{ buffer, Owner::Self };
    }
    else
    {
        if (size == it->second.buffer->size())
        { 
            it->second.owner = Owner::Self;
            it->second.buffer->update_from(*package);
        }
        else
        {
            int id = it->first;
            buffers.erase(it);
            lk.unlock();
            server_requests->free_buffer(id);
            server_requests->allocate_buffer(size, format, usage);
            return;
        }
    }

    if (!promises.empty())
    {
        buffers[protobuf_buffer.buffer_id()].owner = Owner::ContentProducer;
        promises.front().set_value(buffers[protobuf_buffer.buffer_id()].buffer);
        promises.pop_front();
    }
}

void mcl::BufferVault::set_size(geom::Size sz)
{
    std::lock_guard<std::mutex> lk(mutex);
    size = sz;
}
