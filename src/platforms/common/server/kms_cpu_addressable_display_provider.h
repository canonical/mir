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
class CPUAddressableDisplayAllocator : public graphics::CPUAddressableDisplayAllocator
{
public:
    /// Create an CPUAddressableDisplayAllocator if and only if supported by the device
    /// \return the provider, or an empty pointer
    static auto create_if_supported(mir::Fd const& drm_fd, geometry::Size size)
        -> std::shared_ptr<CPUAddressableDisplayAllocator>;

    auto supported_formats() const
        -> std::vector<DRMFormat> override;

    auto alloc_fb(DRMFormat format)
        -> std::unique_ptr<MappableFB> override;

    auto output_size() const -> geometry::Size override;

    auto describe_platform() const -> std::string override;
private:
    explicit CPUAddressableDisplayAllocator(mir::Fd drm_fd, geometry::Size size);

    mir::Fd const drm_fd;
    bool const supports_modifiers;
    geometry::Size const size;
};
}
}
}

#endif //MIR_GRAPHICS_BASIC_CPU_ADDRESSABLE_DISPLAY_PROVIDER_H
