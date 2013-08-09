/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/default_display_configuration_policy.h"
#include "mir/graphics/display_configuration.h"

namespace mg = mir::graphics;
namespace geom = mir::geometry;

void mg::DefaultDisplayConfigurationPolicy::apply_to(DisplayConfiguration& conf)
{
    size_t const preferred_mode_index{0};

    conf.for_each_output(
        [&conf](DisplayConfigurationOutput const& conf_output)
        {
            if (conf_output.connected && conf_output.modes.size() > 0)
            {
                conf.configure_output(conf_output.id, true, geom::Point(),
                                      preferred_mode_index);
            }
            else
            {
                conf.configure_output(conf_output.id, false, conf_output.top_left,
                                      conf_output.current_mode_index);
            }
        });
}

