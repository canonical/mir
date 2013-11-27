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
#include <mir_toolkit/common.h>

namespace mir
{
namespace geometry
{
// XXX temporary
typedef MirPixelFormat PixelFormat;

static inline size_t bytes_per_pixel(PixelFormat fmt)
{
    return (fmt == mir_pixel_format_bgr_888) ? 3 : 4;
}

static inline bool has_alpha(PixelFormat fmt)
{
    return (fmt == mir_pixel_format_abgr_8888) ||
           (fmt == mir_pixel_format_argb_8888);
}

}
}

#endif /* MIR_COMPOSITOR_PIXEL_FORMAT_H_ */
