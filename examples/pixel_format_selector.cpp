/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#include "pixel_format_selector.h"

#include "mir/graphics/display_configuration.h"
#include "mir/graphics/pixel_format_utils.h"

#include <stdexcept>
#include <algorithm>


namespace mir
{
namespace examples
{

PixelFormatSelector::PixelFormatSelector(std::shared_ptr<DisplayConfigurationPolicy> const& base_policy,
                                         bool with_alpha)
    : base_policy{base_policy},
    with_alpha{with_alpha}
{}

void PixelFormatSelector::apply_to(graphics::DisplayConfiguration & conf)
{
    base_policy->apply_to(conf);
    conf.for_each_output(
        [&](graphics::UserDisplayConfigurationOutput& conf_output)
        {
            if (!conf_output.connected || !conf_output.used) return;

            auto const& pos = find_if(conf_output.pixel_formats.begin(),
                                      conf_output.pixel_formats.end(),
                                      [&](MirPixelFormat format) -> bool
                                          {
                                              return graphics::contains_alpha(format) == with_alpha;
                                          }
                                     );

            // keep the default settings if nothing was found
            if (pos == conf_output.pixel_formats.end())
                return;

            conf_output.current_format = *pos;
        });
}

}
}

