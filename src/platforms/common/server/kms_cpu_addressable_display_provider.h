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

#ifndef MIR_GRAPHICS_BASIC_CPU_ADDRESSABLE_DISPLAY_PROVIDER_H
#define MIR_GRAPHICS_BASIC_CPU_ADDRESSABLE_DISPLAY_PROVIDER_H

#include "mir/graphics/platform.h"
#include <mir/fd.h>

namespace mir
{
namespace graphics
{
namespace kms
{
class CPUAddressableDisplayProvider : public graphics::CPUAddressableDisplayProvider
{
public:
    explicit CPUAddressableDisplayProvider(mir::Fd drm_fd);

    auto supported_formats() const
        -> std::vector<DRMFormat> override;

    auto alloc_fb(geometry::Size pixel_size, DRMFormat format)
        -> std::unique_ptr<MappableFB> override;

private:
    mir::Fd const drm_fd;
    bool const supports_modifiers;
};
}
}
}

#endif //MIR_GRAPHICS_BASIC_CPU_ADDRESSABLE_DISPLAY_PROVIDER_H
