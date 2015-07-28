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

#include <memory>
#include <future>

namespace mir
{
namespace protobuf { class Buffer; }
namespace graphics { class BufferProperties; }
namespace client
{
class ServerBufferRequests;
class ClientBufferFactory;
class ClientBuffer;
class BufferVault
{
public:
    BufferVault(
        std::shared_ptr<ClientBufferFactory> const&, std::shared_ptr<ServerBufferRequests> const&,
        unsigned int initial_nbuffers, graphics::BufferProperties const& initial_properties);

    std::future<std::shared_ptr<ClientBuffer>> withdraw();
    void deposit(std::shared_ptr<ClientBuffer> const& buffer);
    void wire_transfer_inbound(protobuf::Buffer const&);
    void wire_transfer_outbound(std::shared_ptr<ClientBuffer> const& buffer);
};
}
}
