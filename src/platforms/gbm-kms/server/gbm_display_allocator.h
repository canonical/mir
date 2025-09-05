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

#include "mir/graphics/platform.h"
#include "mir/fd.h"

namespace mir::graphics::gbm
{
class GBMDisplayAllocator : public graphics::GBMDisplayAllocator
{
public:
    GBMDisplayAllocator(mir::Fd drm_fd, std::shared_ptr<struct gbm_device> gbm, geometry::Size size);

    auto supported_formats() const -> std::vector<DRMFormat> override;

    auto modifiers_for_format(DRMFormat format) const -> std::vector<uint64_t> override;

    auto make_surface(DRMFormat format, std::span<uint64_t> modifier) -> std::unique_ptr<GBMSurface> override;
private:
    mir::Fd const fd;
    std::shared_ptr<struct gbm_device> const gbm;
    geometry::Size const size;
};

using LockedFrontBuffer = std::unique_ptr<gbm_bo, std::function<void(gbm_bo*)>>;

// TODO: Dedup this
class GBMBuffer : public Buffer, public NativeBufferBase
{
public:
    GBMBuffer(mir::Fd drm_fd, LockedFrontBuffer front_buffer);

    BufferID id() const override;

    geometry::Size size() const override;

    MirPixelFormat pixel_format() const override;

    auto to_framebuffer() -> std::unique_ptr<Framebuffer> override;

private:
    LockedFrontBuffer front_buffer;
    mir::Fd const drm_fd;
};
}
