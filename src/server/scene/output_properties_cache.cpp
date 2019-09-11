/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/scene/output_properties_cache.h"

#include "mir/geometry/rectangle.h"
#include "mir/graphics/display_configuration.h"

#include <atomic>
#include <cmath>

namespace ms = mir::scene;
namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace
{
int calculate_dpi(mir::geometry::Size const& resolution, mir::geometry::Size const& size)
{
    float constexpr mm_per_inch = 25.4f;

    auto diagonal_mm = sqrt(size.height.as_int()*size.height.as_int()
                            + size.width.as_int()*size.width.as_int());
    auto diagonal_px = sqrt(resolution.height.as_int() * resolution.height.as_int()
                            + resolution.width.as_int() * resolution.width.as_int());

    return diagonal_px / diagonal_mm * mm_per_inch;
}
}

void ms::OutputPropertiesCache::update_from(mg::DisplayConfiguration const &config)
{
    auto new_properties = std::make_shared<std::vector<OutputProperties>>();

    config.for_each_output(
        [&new_properties](mg::DisplayConfigurationOutput const& output)
        {
            if (output.current_mode_index < output.modes.size())
            {
                auto const mode = output.modes[output.current_mode_index];
                new_properties->emplace_back(OutputProperties {
                    output.extents(),
                    calculate_dpi(mode.size, output.physical_size_mm),
                    output.scale,
                    mode.vrefresh_hz,
                    output.form_factor,
                    output.id});
            }
        });

    std::lock_guard<std::mutex> lk(mutex);
    cache = new_properties;
}

auto ms::OutputPropertiesCache::get_cache() const
    -> std::shared_ptr<std::vector<OutputProperties>>
{
    std::lock_guard<std::mutex> lk(mutex);
    return cache;
}

auto ms::OutputPropertiesCache::properties_for(geom::Rectangle const& extents) const
    -> std::shared_ptr<OutputProperties const>
{
    auto temp_properties = get_cache();

    std::shared_ptr<OutputProperties const> matching_output_properties{};
    if (temp_properties)
    {
        for (auto const &output : *temp_properties)
        {
            if (output.extents.overlaps(extents))
            {
                if (!matching_output_properties ||
                    (output.scale > matching_output_properties->scale))
                {
                    matching_output_properties = std::shared_ptr<OutputProperties const>(
                        temp_properties,
                        &output
                    );
                }
            }
        }
    }

    return matching_output_properties;
}
