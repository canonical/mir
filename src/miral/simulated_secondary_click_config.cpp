/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "miral/simulated_secondary_click_config.h"

#include "mir/log.h"
#include "mir/options/option.h"
#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"

struct miral::SimulatedSecondaryClickConfig::Self
{
    Self(bool enabled_by_default) :
        enabled_by_default{enabled_by_default}
    {
    }

    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
    bool const enabled_by_default;
};

miral::SimulatedSecondaryClickConfig::SimulatedSecondaryClickConfig(bool enabled_by_default) :
    self{std::make_shared<Self>(enabled_by_default)}
{
}

void miral::SimulatedSecondaryClickConfig::enabled(bool enabled) const
{
    if (self->accessibility_manager.expired())
    {
        mir::log_error("AccessibilityManager not initialized. Will not toggle simulated secondary click");
        return;
    }

    self->accessibility_manager.lock()->simulated_secondary_click_enabled(enabled);
}

void miral::SimulatedSecondaryClickConfig::hold_duration(std::chrono::milliseconds delay) const
{
    if (self->accessibility_manager.expired())
    {
        mir::log_error("AccessibilityManager not initialized. Will not update simulated secondary click hold duration");
        return;
    }

    self->accessibility_manager.lock()->simulated_secondary_click_hold_duration(delay);
}

void miral::SimulatedSecondaryClickConfig::operator()(mir::Server& server) const
{
    constexpr auto* enable_simulated_secondary_click_opt = "enable-simulated-secondary-click";
    constexpr auto* simulated_secondary_click_delay_opt = "simulated-secondary-click-delay";

    server.add_configuration_option(
        enable_simulated_secondary_click_opt,
        "Enable simulated secondary click (holding the left button to simulate a right click)",
        self->enabled_by_default);
    server.add_configuration_option(
        simulated_secondary_click_delay_opt,
        "delay required before a left click hold registers as a right click in milliseconds",
        1000);

    server.add_init_callback(
        [&server, this]
        {
            self->accessibility_manager = server.the_accessibility_manager();
            hold_duration(
                std::chrono::milliseconds(server.get_options()->get<int>(simulated_secondary_click_delay_opt)));
            enabled(server.get_options()->get<bool>(enable_simulated_secondary_click_opt));
        });
}
