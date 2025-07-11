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
#include "miral/live_config.h"

#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"
#include "mir/shell/simulated_secondary_click_transformer.h"
#include "mir/synchronised.h"
#include "mir/log.h"

struct miral::SimulatedSecondaryClick::Self
{
    explicit Self(bool enabled_by_default) :
        state{State{enabled_by_default}}
    {
    }

    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;

    struct State
    {
        explicit State(bool enabled) :
            enabled{enabled}
        {
        }

        bool enabled;
        std::chrono::milliseconds hold_duration{1000};
        float displacement_threshold{20.0f};
        std::function<void()> on_hold_start{[] {}};
        std::function<void()> on_hold_cancel{[] {}};
        std::function<void()> on_secondary_click{[] {}};
    };

    mir::Synchronised<State> state;
};

miral::SimulatedSecondaryClick::SimulatedSecondaryClick(bool enabled_by_default) :
    self{std::make_shared<Self>(enabled_by_default)}
{
}

miral::SimulatedSecondaryClick::SimulatedSecondaryClick(live_config::Store& config_store)
{
    config_store.add_bool_attribute(
        {"simulated_secondary_click", "enable"},
        "Whether or not simulated secondary click is enabled",
        [this](live_config::Key const&, std::optional<bool> val)
        {
            if (!val)
                return;

            if (*val)
                this->enable();
            else
                this->disable();
        });

    config_store.add_float_attribute(
        {"simulated_secondary_click", "displacement_threshold"},
        "How much pointer displacement in pixels is allowed before a simulated secondary click is cancelled",
        [this](live_config::Key const& key, std::optional<float> val)
        {
            if (!val)
                return;

            if (*val < 0)
            {
                mir::log_warning(
                    "Config value %s does not support negative values. Ignoring the supplied value (%f)...",
                    key.to_string().c_str(),
                    *val);
                return;
            }

            this->displacement_threshold(*val);
        });

    config_store.add_int_attribute(
        {"simulated_secondary_click", "delay"},
        "The delay in milliseconds between the cursor starting a simulated secondary click and the click being "
        "dispatched.",
        [this](live_config::Key const& key, std::optional<int> val) {
            if (!val)
                return;

            if (*val < 0)
            {
                mir::log_warning(
                    "Config value %s does not support negative values. Ignoring the supplied value (%d)...",
                    key.to_string().c_str(),
                    *val);
                return;
            }

            this->hold_duration(std::chrono::milliseconds{*val});
        });
}

miral::SimulatedSecondaryClick::SimulatedSecondaryClick(std::shared_ptr<Self> self) :
    self{std::move(self)}
{
}

auto miral::SimulatedSecondaryClick::enabled() -> SimulatedSecondaryClick
{
    return SimulatedSecondaryClick{std::make_shared<Self>(true)};
}

auto miral::SimulatedSecondaryClick::disabled() -> SimulatedSecondaryClick
{
    return SimulatedSecondaryClick{std::make_shared<Self>(false)};
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

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::on_hold_start(std::function<void()>&& on_hold_start)
{
    // Server started, forward value to transformer
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click().on_hold_start(std::move(on_hold_start));
    else
        // Store for when the server starts up
        self->state.lock()->on_hold_start = std::move(on_hold_start);

    return *this;
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::on_hold_cancel(std::function<void()>&& on_hold_cancel)
{
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click().on_hold_cancel(std::move(on_hold_cancel));
    else
        self->state.lock()->on_hold_cancel = std::move(on_hold_cancel);

    return *this;
}

miral::SimulatedSecondaryClick& miral::SimulatedSecondaryClick::on_secondary_click(
    std::function<void()>&& on_secondary_click)
{
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->simulated_secondary_click().on_secondary_click(std::move(on_secondary_click));
    else
        self->state.lock()->on_secondary_click = std::move(on_secondary_click);

    return *this;
}

void miral::SimulatedSecondaryClick::operator()(mir::Server& server)
{
    server.add_init_callback(
        [&server, this]
        {
            self->accessibility_manager = server.the_accessibility_manager();
            auto accessibility_manager = server.the_accessibility_manager();

            auto const state = self->state.lock();

            if (state->enabled)
                accessibility_manager->simulated_secondary_click_enabled(true);

            auto& ssc = accessibility_manager->simulated_secondary_click();
            ssc.hold_duration(state->hold_duration);
            ssc.displacement_threshold(state->displacement_threshold);
            ssc.on_hold_start(std::move(state->on_hold_start));
            ssc.on_hold_cancel(std::move(state->on_hold_cancel));
            ssc.on_secondary_click(std::move(state->on_secondary_click));
        });
}
