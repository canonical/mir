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

#ifndef MIR_CLIENT_BUFFER_VAULT_H_
#define MIR_CLIENT_BUFFER_VAULT_H_

#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_native_buffer.h"
#include "mir_wait_handle.h"
#include <memory>
#include "no_tls_future-inl.h"
#include <deque>
#include <map>

namespace mir
{
namespace protobuf { class Buffer; }
namespace client
{
class ClientBuffer;
class MirBuffer;
class AsyncBufferFactory;
class SurfaceMap;

class ServerBufferRequests
{
public:
    virtual void allocate_buffer(geometry::Size size, MirPixelFormat format, int usage) = 0;
    virtual void free_buffer(int buffer_id) = 0;
    virtual void submit_buffer(MirBuffer&) = 0;
    virtual ~ServerBufferRequests() = default;
protected:
    ServerBufferRequests() = default;
    ServerBufferRequests(ServerBufferRequests const&) = delete;
    ServerBufferRequests& operator=(ServerBufferRequests const&) = delete;
};

class ClientBufferFactory;

class BufferVault
{
public:
    BufferVault(
        std::shared_ptr<ClientBufferFactory> const&,
        std::shared_ptr<AsyncBufferFactory> const&,
        std::shared_ptr<ServerBufferRequests> const&,
        std::weak_ptr<SurfaceMap> const&,
        geometry::Size size, MirPixelFormat format, int usage,
        unsigned int initial_nbuffers);
    ~BufferVault();

    NoTLSFuture<std::shared_ptr<MirBuffer>> withdraw();
    void deposit(std::shared_ptr<MirBuffer> const& buffer);
    void wire_transfer_inbound(int buffer_id);
    MirWaitHandle* wire_transfer_outbound(
        std::shared_ptr<MirBuffer> const& buffer, std::function<void()> const&);
    void set_size(geometry::Size);
    void disconnected();
    void set_scale(float scale);
    void set_interval(int);

private:
    enum class Owner;
    typedef std::map<int, Owner> BufferMap;
    BufferMap::iterator available_buffer();
    void trigger_callback(std::unique_lock<std::mutex> lk);

    void alloc_buffer(geometry::Size size, MirPixelFormat format, int usage);
    void free_buffer(int free_id);
    void realloc_buffer(int free_id, geometry::Size size, MirPixelFormat format, int usage);
    std::shared_ptr<MirBuffer> checked_buffer_from_map(int id);
    void set_size(std::unique_lock<std::mutex> const& lk, geometry::Size new_size);


    std::shared_ptr<ClientBufferFactory> const platform_factory;
    std::shared_ptr<AsyncBufferFactory> const buffer_factory;
    std::shared_ptr<ServerBufferRequests> const server_requests;
    std::weak_ptr<SurfaceMap> const surface_map;
    MirPixelFormat const format;
    int const usage;

    std::mutex mutex;
    bool being_destroyed{false};
    BufferMap buffers;
    std::deque<NoTLSPromise<std::shared_ptr<MirBuffer>>> promises;
    geometry::Size size;
    bool disconnected_;
    size_t current_buffer_count;
    size_t needed_buffer_count;
    size_t const initial_buffer_count;
    int last_received_id = 0;
    int interval = 1;
    MirWaitHandle swap_buffers_wait_handle;
    std::function<void()> deferred_cb;
};
}
}
#endif /* MIR_CLIENT_BUFFER_VAULT_H_ */
