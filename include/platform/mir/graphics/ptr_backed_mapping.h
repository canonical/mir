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

#ifndef MIR_PLATFORM_GRAPHICS_PTR_BACKED_MAPPING_H_
#define MIR_PLATFORM_GRAPHICS_PTR_BACKED_MAPPING_H_

#include "mir/renderer/sw/pixel_source.h"
#include "mir_toolkit/common.h"

namespace mir::graphics
{

template<typename T>
class PtrBackedMapping : public renderer::software::Mapping<T>
{
public:
    PtrBackedMapping(
        T* ptr,
        MirPixelFormat format,
        geometry::Size size,
        geometry::Stride stride)
        : ptr{ptr},
          format_{format},
          size_{size},
          stride_{stride}
    {
    }

    auto data() const -> T* override
    {
        return ptr;
    }

    auto len() const -> size_t override
    {
        return
            size().height.as_uint32_t() *
            stride().as_uint32_t() *
            MIR_BYTES_PER_PIXEL(format());
    }

    auto size() const -> geometry::Size override
    {
        return size_;
    }

    auto stride() const -> geometry::Stride override
    {
        return stride_;
    }

    auto format() const -> MirPixelFormat override
    {
        return format_;
    }
private:
    T* const ptr;
    MirPixelFormat const format_;
    geometry::Size const size_;
    geometry::Stride const stride_;
};
}

#endif // MIR_PLATFORM_GRAPHICS_PTR_BACKED_MAPPING_H_
