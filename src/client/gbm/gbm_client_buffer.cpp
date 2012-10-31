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
 * Authored by: Kevin DuBois<kevin.dubois@canonical.com>
 */

#include "mir_client/mir_client_library.h"
#include "mir_client/gbm/gbm_client_buffer.h"

namespace mcl=mir::client;
namespace geom=mir::geometry;

mcl::GBMClientBuffer::GBMClientBuffer(
                        std::shared_ptr<MirBufferPackage> && package,
                        geom::Size size, geom::PixelFormat pf)
 : creation_package(std::move(package)),
   rect({{geom::X(0),geom::Y(0)}, size}),
   buffer_pf(pf)
{
}

std::shared_ptr<mcl::MemoryRegion> mcl::GBMClientBuffer::secure_for_cpu_write()
{
    return std::shared_ptr<mcl::MemoryRegion>();
}

geom::Size mcl::GBMClientBuffer::size() const
{
    return rect.size;
}

geom::Stride mcl::GBMClientBuffer::stride() const
{
    return geom::Stride{creation_package->stride};
}

geom::PixelFormat mcl::GBMClientBuffer::pixel_format() const
{
    return buffer_pf;
}

std::shared_ptr<MirBufferPackage> mcl::GBMClientBuffer::get_buffer_package() const
{
    return creation_package;
}
