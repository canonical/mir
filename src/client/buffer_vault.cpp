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

#include "buffer_vault.h"

namespace mcl = mir::client;
namespace mp = mir::protobuf;
namespace mg = mir::graphics;

mcl::BufferVault::BufferVault(
    std::shared_ptr<ClientBufferFactory> const&, std::shared_ptr<ServerBufferRequests> const&,
    unsigned int initial_nbuffers, mg::BufferProperties const& initial_properties)
{
    (void) initial_nbuffers;
    (void) initial_properties;
}

std::future<std::shared_ptr<mcl::ClientBuffer>> mcl::BufferVault::withdraw()
{
    std::promise<std::shared_ptr<mcl::ClientBuffer>> promise;
    promise.set_value(nullptr);
    return promise.get_future();
}

void mcl::BufferVault::deposit(std::shared_ptr<mcl::ClientBuffer> const& buffer)
{
    (void) buffer;
}

void mcl::BufferVault::wire_transfer_inbound(protobuf::Buffer const&)
{
}

void mcl::BufferVault::wire_transfer_outbound(std::shared_ptr<mcl::ClientBuffer> const& buffer)
{
    (void) buffer;
}
