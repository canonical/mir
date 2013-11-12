/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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
 *   Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_GRAPHICS_GBM_GBM_BUFFER_H_
#define MIR_GRAPHICS_GBM_GBM_BUFFER_H_

#include "mir/graphics/buffer_basic.h"

#include <gbm.h>

#include <memory>
#include <limits>

namespace mir
{
namespace graphics
{
namespace gbm
{

struct GBMNativeBuffer : MirNativeBuffer
{
    struct gbm_bo *bo;
};

geometry::PixelFormat gbm_format_to_mir_format(uint32_t format);
uint32_t mir_format_to_gbm_format(geometry::PixelFormat format);
enum : uint32_t { invalid_gbm_format = std::numeric_limits<uint32_t>::max() };

class BufferTextureBinder;

class GBMBuffer: public BufferBasic
{
public:
    GBMBuffer(std::shared_ptr<gbm_bo> const& handle,
              uint32_t bo_flags,
              std::unique_ptr<BufferTextureBinder> texture_binder);
    GBMBuffer(const GBMBuffer&) = delete;
    ~GBMBuffer();

    GBMBuffer& operator=(const GBMBuffer&) = delete;

    virtual geometry::Size size() const;

    virtual geometry::Stride stride() const;

    virtual geometry::PixelFormat pixel_format() const;

    virtual std::shared_ptr<MirNativeBuffer> native_buffer_handle() const;

    virtual void bind_to_texture();

    bool can_bypass() const override;

private:
    std::shared_ptr<gbm_bo> const gbm_handle;
    uint32_t bo_flags;
    std::unique_ptr<BufferTextureBinder> const texture_binder;
    int prime_fd;
};

}
}
}


#endif // MIR_GRAPHICS_GBM_GBM_BUFFER_H_
