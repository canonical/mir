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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by:
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "gbm_client_buffer_factory.h"
#include "gbm_client_buffer.h"

namespace mcl=mir::client;
namespace mclg=mir::client::gbm;

mclg::GBMClientBufferFactory::GBMClientBufferFactory(
        std::shared_ptr<DRMFDHandler> const& drm_fd_handler)
    : drm_fd_handler{drm_fd_handler}
{
}

std::shared_ptr<mcl::ClientBuffer> mclg::GBMClientBufferFactory::create_buffer(std::shared_ptr<MirBufferPackage> const& package, geometry::Size size, geometry::PixelFormat pf)
{
    (void)size; // Deprecated. Use dimensions from package instead.
    return std::make_shared<mclg::GBMClientBuffer>(drm_fd_handler, package, pf);
}
