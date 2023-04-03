/*
 * Copyright © 2021 Canonical Ltd.
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

#ifndef MIR_GRAPHICS_GBM_FB_H_
#define MIR_GRAPHICS_GBM_FB_H_

#include "mir/fd.h"
#include "mir/graphics/platform.h"

#include "kms_framebuffer.h"

namespace mir::graphics::gbm
{
class DumbFB : public FBHandle, public DumbDisplayProvider::MappableFB
{
public:
    DumbFB(mir::Fd const& drm_fd, bool supports_modifiers, mir::geometry::Size const& size);
    ~DumbFB() override;

    auto map_writeable() -> std::unique_ptr<mir::renderer::software::Mapping<unsigned char>> override;

    auto format() const -> MirPixelFormat override;
    auto stride() const -> geometry::Stride override;
    auto size() const -> geometry::Size override; 

    operator uint32_t() const override;
    
    DumbFB(DumbFB const&) = delete;
    DumbFB& operator=(DumbFB const&) = delete;
private:
    class DumbBuffer;

    DumbFB(mir::Fd drm_fd, bool supports_modifiers, std::unique_ptr<DumbBuffer> buffer);
    static auto fb_id_for_buffer(mir::Fd const& drm_fd, bool supports_modifiers, DumbBuffer const& buf) -> uint32_t;

    mir::Fd const drm_fd;
    uint32_t const fb_id;
    std::unique_ptr<DumbBuffer> const buffer;
};

}

#endif //MIR_GRAPHICS_GBM_FB_H_
