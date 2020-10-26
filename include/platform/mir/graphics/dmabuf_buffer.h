/*
 * Copyright © 2020 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_GRAPHICS_DMABUF_BUFFER_H_
#define MIR_GRAPHICS_DMABUF_BUFFER_H_

#include <cstdint>
#include <optional>

#include "mir/graphics/buffer.h"
#include "mir/fd.h"

namespace mir
{
namespace graphics
{
/**
 * A logical buffer backed by one-or-more dmabuf buffers
 */
class DMABufBuffer
{
public:
    struct PlaneDescriptor
    {
        mir::Fd dma_buf;
        uint32_t stride;
        uint32_t offset;
    };
    virtual ~DMABufBuffer() = default;

    /**
     * The format of this logical buffer, as in <drm_fourcc.h>
     */
    virtual auto drm_fourcc() const -> uint32_t = 0;

    /**
     * The DRM modifier of this logical buffer, if specified.
     * \note    Both the DRM and Wayland APIs accept per-plane modifiers. However, this
     *          doesn't actually make sense (the modifier modifies the *logical* buffer
     *          format) and the kernel rejects any request where there are different
     *          modifiers for different planes.
     */
    virtual auto modifier() const -> std::optional<uint64_t> = 0;

    virtual auto planes() const -> std::vector<PlaneDescriptor> const& = 0;

    virtual auto size() const -> geometry::Size = 0;
};
}
}

#endif //MIR_GRAPHICS_DMABUF_BUFFER_H_
