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

#include "kms_cpu_addressable_display_provider.h"
#include "cpu_addressable_fb.h"
#include <drm_fourcc.h>
#include <xf86drm.h>

namespace
{
auto drm_get_cap_checked(mir::Fd const& drm_fd, uint64_t cap) -> uint64_t
{
    uint64_t value;
    if (drmGetCap(drm_fd, cap, &value))
    {
        BOOST_THROW_EXCEPTION((
            std::system_error{
                errno,
                std::system_category(),
                "Failed to query DRM capabilities"}));
    }
    return value;
}
}

namespace mg = mir::graphics;
namespace geom = mir::geometry;

mg::kms::CPUAddressableDisplayAllocator::CPUAddressableDisplayAllocator(mir::Fd drm_fd, geom::Size size)
    : drm_fd{std::move(drm_fd)},
      supports_modifiers{drm_get_cap_checked(this->drm_fd, DRM_CAP_ADDFB2_MODIFIERS) == 1},
      size{size}
{
}

auto mg::kms::CPUAddressableDisplayAllocator::supported_formats() const
-> std::vector<mg::DRMFormat>
{
    // TODO: Pull out of DRM info
    return {mg::DRMFormat{DRM_FORMAT_XRGB8888}, mg::DRMFormat{DRM_FORMAT_ARGB8888}};
}

auto mg::kms::CPUAddressableDisplayAllocator::alloc_buffer(DRMFormat format) 
-> std::unique_ptr<MappableBuffer>
{
    return std::make_unique<mg::CPUAddressableBuffer>(drm_fd, supports_modifiers, format, size);
}

auto mg::kms::CPUAddressableDisplayAllocator::output_size() const -> geom::Size
{
    return size;
}

auto mir::graphics::kms::CPUAddressableDisplayAllocator::create_if_supported(mir::Fd const& drm_fd, geom::Size size)
-> std::shared_ptr<CPUAddressableDisplayAllocator>
{
    if  (drm_get_cap_checked(drm_fd, DRM_CAP_DUMB_BUFFER))
    {
        return std::shared_ptr<CPUAddressableDisplayAllocator>(new CPUAddressableDisplayAllocator{drm_fd, size});
    }
    else
    {
        return {};
    }
}
