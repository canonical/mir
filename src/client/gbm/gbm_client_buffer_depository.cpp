/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by:
 *   Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "gbm_client_buffer_depository.h"
#include "gbm_client_buffer.h"

#include <stdexcept>
#include <map>

namespace mcl=mir::client;
namespace mclg=mir::client::gbm;

mclg::GBMClientBufferDepository::GBMClientBufferDepository(
        std::shared_ptr<DRMFDHandler> const& drm_fd_handler)
    : current_buffer(buffers.end()), drm_fd_handler{drm_fd_handler}
{
}

void mclg::GBMClientBufferDepository::deposit_package(std::shared_ptr<mir_toolkit::MirBufferPackage>&& package, int id, geometry::Size size, geometry::PixelFormat pf)
{
    for (auto next = buffers.begin(); next != buffers.end();)
    {
        // C++ guarantees that buffers.erase() will only invalidate the iterator we
        // erase. Move to the next buffer before we potentially invalidate our iterator.
        auto current = next++;

        if (current != current_buffer &&
            current->first != id &&
            current->second->age() >= 2)
        {
            buffers.erase(current);
        }
    }

    for (auto& current : buffers)
    {
        current.second->increment_age();
    }

    if (current_buffer != buffers.end())
    {
        current_buffer->second->mark_as_submitted();
    }

    current_buffer = buffers.find(id);

    if (current_buffer == buffers.end())
    {
        auto new_buffer = std::make_shared<mclg::GBMClientBuffer>(drm_fd_handler, std::move(package), size, pf);

        current_buffer = buffers.insert(current_buffer, std::make_pair(id, new_buffer));
    }
}

std::shared_ptr<mcl::ClientBuffer> mclg::GBMClientBufferDepository::access_current_buffer()
{
    return current_buffer->second;
}
