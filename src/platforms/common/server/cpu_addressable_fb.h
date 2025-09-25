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

#ifndef MIR_GRAPHICS_CPU_ADDRESSABLE_FB_H_
#define MIR_GRAPHICS_CPU_ADDRESSABLE_FB_H_

#include "mir/fd.h"
#include "mir/graphics/platform.h"

#include "kms_framebuffer.h"

namespace mir::graphics
{
class CPUAddressableFB : public FBHandle, public CPUAddressableDisplayAllocator::MappableFB
{
public:
    CPUAddressableFB(
        mir::Fd const& drm_fd,
        bool supports_modifiers,
        DRMFormat format,
        mir::geometry::Size const& size);
    ~CPUAddressableFB() override;

    auto map_writeable() -> std::unique_ptr<mir::renderer::software::Mapping<unsigned char>> override;

    auto format() const -> MirPixelFormat override;
    auto stride() const -> geometry::Stride override;
    auto size() const -> geometry::Size override;

    operator uint32_t() const override;

    CPUAddressableFB(CPUAddressableFB const&) = delete;
    CPUAddressableFB& operator=(CPUAddressableFB const&) = delete;
private:
    class Buffer;

    CPUAddressableFB(
        mir::Fd drm_fd,
        bool supports_modifiers,
        DRMFormat format,
        std::unique_ptr<Buffer> buffer);
    static auto fb_id_for_buffer(
        mir::Fd const& drm_fd,
        bool supports_modifiers,
        DRMFormat format,
        Buffer const& buf) -> uint32_t;

    mir::Fd const drm_fd;
    uint32_t const fb_id;
    std::unique_ptr<Buffer> const buffer;
};

}

#endif //MIR_GRAPHICS_CPU_ADDRESSABLE_FB_H_
