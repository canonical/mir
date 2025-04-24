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

#include "miral/simulated_secondary_click.h"

#include "mir/options/option.h"
#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"
#include "mir/shell/simulated_secondary_click_transformer.h"
#include "mir/synchronised.h"

struct miral::SimulatedSecondaryClick::Self
{
    Self(bool enabled_by_default) :
        state{State{enabled_by_default}}
    {
    }

    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;

    struct State
    {
        State(bool enabled) :
            enabled{enabled}
        {
        }

        bool enabled;
        std::chrono::milliseconds hold_duration{1000};
        float displacement_threshold{20.0f};
        std::function<void()> on_enabled, on_disabled, on_hold_start, on_hold_cancel, on_secondary_click;
    };

    mir::Synchronised<State> state;
};

miral::SimulatedSecondaryClick::SimulatedSecondaryClick(bool enabled_by_default) :
    self{std::make_shared<Self>(enabled_by_default)}
{
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::enable()
{
    self->state.lock()->enabled = true;
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click_enabled(true);

    return *this;
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::disable()
{
    self->state.lock()->enabled = false;
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click_enabled(false);

    return *this;
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::hold_duration(
    std::chrono::milliseconds hold_duration)
{
    self->state.lock()->hold_duration = hold_duration;
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click().hold_duration(hold_duration);

    return *this;
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::displacement_threshold(float displacement)
{
    self->state.lock()->displacement_threshold = displacement;
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click().displacement_threshold(displacement);

    return *this;
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::on_enabled(std::function<void()>&& on_enabled)
{
    auto const state = self->state.lock();
    state->on_enabled = std::move(on_enabled);
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click().on_enabled(std::move(state->on_enabled));

    return *this;
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::on_disabled(
    std::function<void()>&& on_disabled)
{
    auto const state = self->state.lock();
    state->on_disabled = std::move(on_disabled);
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click().on_disabled(std::move(state->on_disabled));

    return *this;
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::on_hold_start(
    std::function<void()>&& on_hold_start)
{
    auto const state = self->state.lock();
    state->on_hold_start = std::move(on_hold_start);
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click().on_hold_start(std::move(state->on_hold_start));

    return *this;
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::on_hold_cancel(
    std::function<void()>&& on_hold_cancel)
{
    auto const state = self->state.lock();
    state->on_hold_cancel = std::move(on_hold_cancel);
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click().on_hold_cancel(std::move(state->on_hold_cancel));

    return *this;
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::on_secondary_click(
    std::function<void()>&& on_secondary_click)
{
    auto const state = self->state.lock();
    state->on_secondary_click = std::move(on_secondary_click);
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click().on_secondary_click(std::move(state->on_secondary_click));

    return *this;
}

void miral::SimulatedSecondaryClick::operator()(mir::Server& server)
{
    constexpr auto* enable_simulated_secondary_click_opt = "enable-simulated-secondary-click";
    constexpr auto* simulated_secondary_click_delay_opt = "simulated-secondary-click-delay";
    constexpr auto* simulated_secondary_click_displacement_threshold_opt =
        "simulated-secondary-click-displacement-threshold";

    {
        auto const state = self->state.lock();
        server.add_configuration_option(
            enable_simulated_secondary_click_opt,
            "Enable simulated secondary click (holding the left button to simulate a right click)",
            state->enabled);
        server.add_configuration_option(
            simulated_secondary_click_delay_opt,
            "delay required before a left click hold registers as a right click in milliseconds",
            static_cast<int>(state->hold_duration.count()));
        server.add_configuration_option(
            simulated_secondary_click_displacement_threshold_opt,
            "The displacement the cursor pointer is allowed before the simulated secondary click is cancelled",
            state->displacement_threshold);
    }

    server.add_init_callback(
        [&server, this]
        {
            self->accessibility_manager = server.the_accessibility_manager();

            // The default values for these options were set from `self->state`
            // when the options are added a few lines ago.
            hold_duration(
                std::chrono::milliseconds(server.get_options()->get<int>(simulated_secondary_click_delay_opt)));
            displacement_threshold(
                server.get_options()->get<double>(simulated_secondary_click_displacement_threshold_opt));

            if (auto& on_enabled_ = self->state.lock()->on_enabled)
                on_enabled(std::move(on_enabled_));
            if (auto& on_disabled_ = self->state.lock()->on_disabled)
                on_disabled(std::move(on_disabled_));
            if (auto& on_hold_start_ = self->state.lock()->on_hold_start)
                on_hold_start(std::move(on_hold_start_));
            if (auto& on_hold_cancel_ = self->state.lock()->on_hold_cancel)
                on_hold_cancel(std::move(on_hold_cancel_));
            if (auto& on_secondary_click_ = self->state.lock()->on_secondary_click)
                on_secondary_click(std::move(on_secondary_click_));

            if(server.get_options()->get<bool>(enable_simulated_secondary_click_opt))
                enable();
        });
}
