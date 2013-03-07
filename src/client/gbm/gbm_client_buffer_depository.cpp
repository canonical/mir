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
    : drm_fd_handler{drm_fd_handler}
{
}

void mclg::GBMClientBufferDepository::deposit_package(std::shared_ptr<mir_toolkit::MirBufferPackage>&& package, int id, geometry::Size size, geometry::PixelFormat pf)
{
    auto buffer_it = buffers.begin();
    while (buffer_it != buffers.end())
    {
        auto tmp_it = buffer_it;
        ++buffer_it;
        if (tmp_it->first == current_buffer)
        {
            tmp_it->second->mark_as_submitted();
        }
        else
        {
            if (tmp_it->second->age() >= 3)
            {
                // C++ guarantees that buffers.erase() will only invalidate the iterator
                // we erase.
                buffers.erase(tmp_it);
            }
            else
            {
                tmp_it->second->increment_age();
            }
        }
    }
    if (buffers.count(id) == 0)
        buffers[id] = std::make_shared<mclg::GBMClientBuffer>(drm_fd_handler, std::move(package), size, pf);
    current_buffer = id;
}

std::shared_ptr<mcl::ClientBuffer> mclg::GBMClientBufferDepository::access_current_buffer(void)
{
    return buffers[current_buffer];
}
