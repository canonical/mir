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

#include <unordered_map>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

void mg::DefaultDisplayConfigurationPolicy::apply_to(DisplayConfiguration& conf)
{
    static MirPowerMode const default_power_state = mir_power_mode_on;
    std::unordered_map<DisplayConfigurationCardId, size_t> available_outputs_for_card;

    conf.for_each_card(
        [&](DisplayConfigurationCard const& card)
        {
            available_outputs_for_card[card.id] = card.max_simultaneous_outputs;
        });

    conf.for_each_output(
        [&](DisplayConfigurationOutput const& conf_output)
        {
            if (conf_output.connected && conf_output.modes.size() > 0 &&
                available_outputs_for_card[conf_output.card_id] > 0)
            {
                size_t preferred_mode_index{conf_output.preferred_mode_index};
                if (preferred_mode_index > conf_output.modes.size())
                    preferred_mode_index = 0;

                conf.configure_output(conf_output.id, true, geom::Point(),
                                      preferred_mode_index, default_power_state);

                --available_outputs_for_card[conf_output.card_id];
            }
            else
            {
                conf.configure_output(conf_output.id, false, conf_output.top_left,
                                      conf_output.current_mode_index, default_power_state);
            }
        });
}

