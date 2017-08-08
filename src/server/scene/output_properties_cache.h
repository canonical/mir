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

#ifndef MIR_SCENE_OUTPUT_PROPERTIES_CACHE_H_
#define MIR_SCENE_OUTPUT_PROPERTIES_CACHE_H_

#include "mir_toolkit/common.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/display_configuration.h"

#include <vector>
#include <memory>
#include <mutex>

namespace mir
{

namespace scene
{
struct OutputProperties
{
    geometry::Rectangle extents;
    int dpi;
    float scale;
    double refresh_rate;
    MirFormFactor form_factor;
    graphics::DisplayConfigurationOutputId id;
};

class OutputPropertiesCache
{
public:

    void update_from(graphics::DisplayConfiguration const& config);

    std::shared_ptr<OutputProperties const> properties_for(geometry::Rectangle const& extents) const;

private:
    std::shared_ptr<std::vector<OutputProperties>> get_cache() const;

    std::mutex mutable mutex;
    std::shared_ptr<std::vector<OutputProperties>> cache;
};

}
}



#endif //MIR_SCENE_OUTPUT_PROPERTIES_CACHE_H_
