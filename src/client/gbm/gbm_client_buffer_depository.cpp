/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
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

void mclg::GBMClientBufferDepository::deposit_package(std::shared_ptr<mir_toolkit::MirBufferPackage>&& package, int, geometry::Size size, geometry::PixelFormat pf)
{
    buffer = std::make_shared<mclg::GBMClientBuffer>(drm_fd_handler, std::move(package), size, pf);
}

std::shared_ptr<mcl::ClientBuffer> mclg::GBMClientBufferDepository::access_buffer(int)
{
    return buffer;
}

