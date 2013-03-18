/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "client_buffer.h"
#include "client_buffer_factory.h"
#include "client_buffer_depository.h"

#include <stdexcept>
#include <memory>
#include <map>

namespace mcl=mir::client;

mcl::ClientBufferDepository::ClientBufferDepository(std::shared_ptr<ClientBufferFactory> const &factory, int max_buffers)
    : factory(factory),
      current_buffer_iter(buffers.end()),
      max_age(max_buffers - 1)
{
}

void mcl::ClientBufferDepository::deposit_package(std::shared_ptr<mir_toolkit::MirBufferPackage> const & package, int id, geometry::Size size, geometry::PixelFormat pf)
{
    for (auto next = buffers.begin(); next != buffers.end();)
    {
        // C++ guarantees that buffers.erase() will only invalidate the iterator we
        // erase. Move to the next buffer before we potentially invalidate our iterator.
        auto current = next;
        next++;

        if (current != current_buffer_iter &&
            current->first != id &&
            current->second->age() >= max_age)
        {
            buffers.erase(current);
        }
    }

    for (auto& current : buffers)
    {
        current.second->increment_age();
    }

    if (current_buffer_iter != buffers.end())
    {
        current_buffer_iter->second->mark_as_submitted();
    }

    current_buffer_iter = buffers.find(id);

    if (current_buffer_iter == buffers.end())
    {
        auto new_buffer = factory->create_buffer(package, size, pf);

        current_buffer_iter = buffers.insert(current_buffer_iter, std::make_pair(id, new_buffer));
    }
}

std::shared_ptr<mcl::ClientBuffer> mcl::ClientBufferDepository::current_buffer()
{
    return current_buffer_iter->second;
}
