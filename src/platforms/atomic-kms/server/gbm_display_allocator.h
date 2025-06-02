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

namespace mir::graphics::atomic
{
class GbmWorkarounds;
class GBMDisplayAllocator : public graphics::GBMDisplayAllocator
{
public:
    GBMDisplayAllocator(
        mir::Fd drm_fd,
        std::shared_ptr<struct gbm_device> gbm,
        geometry::Size size,
        std::shared_ptr<GbmWorkarounds> runtime_quirks);

    auto supported_formats() const -> std::vector<DRMFormat> override;

    auto modifiers_for_format(DRMFormat format) const -> std::vector<uint64_t> override;

    auto make_surface(DRMFormat format, std::span<uint64_t> modifier) -> std::unique_ptr<GBMSurface> override;
private:
    mir::Fd const fd;
    std::shared_ptr<struct gbm_device> const gbm;
    geometry::Size const size;
    std::shared_ptr<GbmWorkarounds> const runtime_quirks;
};
}
