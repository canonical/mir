/*
 * Copyright © 2015 Canonical Ltd.
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
 *   Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "client_buffer_factory.h"
#include "client_buffer.h"
#include "../debug.h"

namespace mcl=mir::client;
namespace mclx=mir::client::X;

mclx::ClientBufferFactory::ClientBufferFactory(
    std::shared_ptr<BufferFileOps> const& buffer_file_ops)
    : buffer_file_ops{buffer_file_ops}
{
    CALLED
}

std::shared_ptr<mcl::ClientBuffer>
mclx::ClientBufferFactory::create_buffer(
    std::shared_ptr<MirBufferPackage> const& package,
    geometry::Size size,
    MirPixelFormat pf)
{
    (void)size; // TODO: remove this unused parameter

    CALLED

    return std::make_shared<mclx::ClientBuffer>(
        buffer_file_ops,
        package,
        geometry::Size{package->width, package->height},
        pf);
}
