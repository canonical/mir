/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/client_buffer_factory.h"
#include "buffer_vault.h"
#include "mir_protobuf.pb.h"
#include <algorithm>
#include <boost/throw_exception.hpp>

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace geom = mir::geometry;
namespace mp = mir::protobuf;

mcl::BufferVault::BufferVault(
    std::shared_ptr<ClientBufferFactory> const& client_buffer_factory,
    std::shared_ptr<ServerBufferRequests> const& server_requests,
    geom::Size size, MirPixelFormat format, int usage, unsigned int initial_nbuffers) :
    factory(client_buffer_factory),
    server_requests(server_requests)
{
    for (auto i = 0u; i < initial_nbuffers; i++)
        server_requests->allocate_buffer(size, format, usage);
}

mcl::BufferVault::~BufferVault()
{
    for(auto& promise : promises)
        promise.set_exception(make_exception_ptr(
            std::future_error(std::future_errc::broken_promise)));
}

std::future<std::shared_ptr<mcl::ClientBuffer>> mcl::BufferVault::withdraw()
{
    std::promise<std::shared_ptr<mcl::ClientBuffer>> promise;

    printf("BUFFER SIZE %i\n", (int) buffers.size());
    auto it = std::find_if(buffers.begin(), buffers.end(),
        [](std::pair<int, BufferEntry> const& entry) {
            printf("SCAN.\n");
            return !entry.second.server_owned && !entry.second.driver_used;
        });

    auto the_future = promise.get_future();
    if (it != buffers.end())
    {
        it->second.driver_used = true;
        promise.set_value(it->second.buffer);
    }
    else
    {
        printf("DIDNT FIND.\n");
        promises.emplace_back(std::move(promise));
    }
    return the_future;
}

void mcl::BufferVault::deposit(std::shared_ptr<mcl::ClientBuffer> const& buffer)
{
    auto it = std::find_if(buffers.begin(), buffers.end(),
        [&buffer](std::pair<int, BufferEntry> const& entry) {
            return buffer == entry.second.buffer;
        });
    if (it == buffers.end())
    {
        //throw?
    }
    else
    {
        it->second.deposited = true;
    }
    (void) buffer;
}

void mcl::BufferVault::wire_transfer_outbound(std::shared_ptr<mcl::ClientBuffer> const& buffer)
{
    auto it = std::find_if(buffers.begin(), buffers.end(),
        [&buffer](std::pair<int, BufferEntry> const& entry) {
            return buffer == entry.second.buffer;
        });
    if (it == buffers.end() || !it->second.deposited)
        BOOST_THROW_EXCEPTION(std::logic_error("no.\n"));

    it->second.server_owned = true;
    server_requests->submit_buffer();
}

void mcl::BufferVault::wire_transfer_inbound(
    mp::Buffer const& protobuf_buffer, MirPixelFormat pf)
{
    //first track buffer
    auto buffer_package = std::make_shared<MirBufferPackage>();
    buffer_package->data_items = protobuf_buffer.data_size();
    buffer_package->fd_items = protobuf_buffer.fd_size();

    for (int i = 0; i != protobuf_buffer.data_size(); ++i)
        buffer_package->data[i] = protobuf_buffer.data(i);
    for (int i = 0; i != protobuf_buffer.fd_size(); ++i)
        buffer_package->fd[i] = protobuf_buffer.fd(i);

    buffer_package->stride = protobuf_buffer.stride();
    buffer_package->flags = protobuf_buffer.flags();
    buffer_package->width = protobuf_buffer.width();
    buffer_package->height = protobuf_buffer.height();

    auto it = buffers.find(protobuf_buffer.buffer_id());
    if (it == buffers.end())
    {
        buffers[protobuf_buffer.buffer_id()] = 
            BufferEntry{
                factory->create_buffer(buffer_package, geom::Size{buffer_package->width, buffer_package->height}, pf),
            false, false, false 
        };
    }
    else
    {
        it->second.server_owned = false;
        it->second.deposited = false;
        it->second.driver_used = false;
    }

    if (!promises.empty())
    {
        promises.front().set_value(buffers[protobuf_buffer.buffer_id()].buffer);
        promises.pop_front();
    }
}

