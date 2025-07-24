/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mir/graphics/graphic_buffer_allocator.h"

namespace mg = mir::graphics;
namespace geom = mir::geometry;

class mg::BufferParams::Impl
{
public:
    friend class BufferParams;

    geometry::Size size;
    std::optional<DRMFormat> format;
    std::optional<geom::Stride> stride;
};

mg::BufferParams::BufferParams(geom::Size size)
    : impl{std::make_unique<Impl>()}
{
    impl->size = size;
}

auto mg::BufferParams::with_format(DRMFormat format) -> BufferParams&
{
    impl->format = format;
    return *this;
}

auto mg::BufferParams::with_stride(geom::Stride stride) -> BufferParams&
{
    impl->stride = stride;
    return *this;
}

auto mg::BufferParams::size() const -> geom::Size
{
    return impl->size;
}

auto mg::BufferParams::format() const -> std::optional<DRMFormat>
{
    return impl->format;
}

auto mg::BufferParams::stride() const -> std::optional<geom::Stride>
{
    return impl->stride;
}
