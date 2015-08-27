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

#ifndef MIR_CLIENT_BUFFER_VAULT_H_
#define MIR_CLIENT_BUFFER_VAULT_H_

#include "mir/geometry/size.h"
#include "mir_toolkit/common.h"
#include "mir_toolkit/mir_native_buffer.h"
#include <memory>
#include "no_tls_future-inl.h"
#include <deque>
#include <map>

namespace mir
{
namespace protobuf { class Buffer; }
namespace client
{
class ServerBufferRequests
{
public:
    virtual void allocate_buffer(geometry::Size size, MirPixelFormat format, int usage) = 0;
    virtual void free_buffer(int buffer_id) = 0;
    virtual void submit_buffer(ClientBuffer&) = 0;
    virtual ~ServerBufferRequests() = default;
protected:
    ServerBufferRequests() = default;
    ServerBufferRequests(ServerBufferRequests const&) = delete;
    ServerBufferRequests& operator=(ServerBufferRequests const&) = delete;
};

class ClientBufferFactory;
class ClientBuffer;
class BufferVault
{
public:
    BufferVault(
        std::shared_ptr<ClientBufferFactory> const&,
        std::shared_ptr<ServerBufferRequests> const&,
        geometry::Size size, MirPixelFormat format, int usage,
        unsigned int initial_nbuffers);
    ~BufferVault();

    NoTLSFuture<std::shared_ptr<ClientBuffer>> withdraw();
    void deposit(std::shared_ptr<ClientBuffer> const& buffer);
    void wire_transfer_inbound(protobuf::Buffer const&);
    void wire_transfer_outbound(std::shared_ptr<ClientBuffer> const& buffer);
    void set_size(geometry::Size);

private:
    std::shared_ptr<ClientBufferFactory> const factory;
    std::shared_ptr<ServerBufferRequests> const server_requests;
    MirPixelFormat const format;
    int const usage;

    enum class Owner;
    struct BufferEntry
    {
        std::shared_ptr<ClientBuffer> buffer;
        Owner owner;
    };

    std::mutex mutex;
    std::map<int, BufferEntry> buffers;
    std::deque<NoTLSPromise<std::shared_ptr<ClientBuffer>>> promises;
    geometry::Size size;
};
}
}
#endif /* MIR_CLIENT_BUFFER_VAULT_H_ */
