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
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#ifndef MIR_COMPOSITOR_PIXEL_FORMAT_H_
#define MIR_COMPOSITOR_PIXEL_FORMAT_H_

#include <cstdint>
#include <cstddef>

namespace mir
{
namespace geometry
{
enum class PixelFormat : uint32_t
{
    invalid,
    abgr_8888,
    xbgr_8888,
    argb_8888,
    xrgb_8888,
    bgr_888
};

static inline size_t bytes_per_pixel(PixelFormat fmt)
{
    return (fmt == PixelFormat::bgr_888) ? 3 : 4;
}

static inline bool has_alpha(PixelFormat fmt)
{
    return (fmt == PixelFormat::abgr_8888) ||
           (fmt == PixelFormat::argb_8888);
}

}
}

#endif /* MIR_COMPOSITOR_PIXEL_FORMAT_H_ */
