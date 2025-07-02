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

#include "miral/slow_keys.h"
#include "mir/options/option.h"
#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"
#include "mir/shell/slow_keys_transformer.h"

#include "mir/synchronised.h"

struct miral::SlowKeys::Self
{
    explicit Self(bool enabled) :
        state{State{enabled}}
    {
    }

    struct State
    {
        explicit State(bool enabled) :
            enabled{enabled}
        {
        }

        bool enabled;
        std::chrono::milliseconds hold_delay{200};
        std::function<void(unsigned int)> on_key_down{[](auto){}};
        std::function<void(unsigned int)> on_key_rejected{[](auto){}};
        std::function<void(unsigned int)> on_key_accepted{[](auto){}};
    };

    mir::Synchronised<State> state;
    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
};

auto miral::SlowKeys::enabled() -> SlowKeys
{
    return SlowKeys{std::make_shared<Self>(true)};
}

auto miral::SlowKeys::disabled() -> SlowKeys
{
    return SlowKeys{std::make_shared<Self>(false)};
}

miral::SlowKeys::SlowKeys(std::shared_ptr<Self> s) :
    self{std::move(s)}
{
}

miral::SlowKeys& miral::SlowKeys::enable()
{
    self->state.lock()->enabled = true;
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->slow_keys_enabled(true);

    return *this;
}

miral::SlowKeys& miral::SlowKeys::disable()
{
    self->state.lock()->enabled = false;
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->slow_keys_enabled(false);

    return *this;
}

miral::SlowKeys& miral::SlowKeys::hold_delay(std::chrono::milliseconds hold_delay)
{
    self->state.lock()->hold_delay = hold_delay;
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->slow_keys().delay(hold_delay);

    return *this;
}

miral::SlowKeys& miral::SlowKeys::on_key_down(std::function<void(unsigned int)>&& on_key_down)
{
    auto const state = self->state.lock();
    state->on_key_down = std::move(on_key_down);
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->slow_keys().on_key_down(std::move(state->on_key_down));

    return *this;
}

miral::SlowKeys& miral::SlowKeys::on_key_rejected(std::function<void(unsigned int)>&& on_key_rejected)
{
    auto const state = self->state.lock();
    state->on_key_rejected = std::move(on_key_rejected);
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->slow_keys().on_key_rejected(std::move(state->on_key_rejected));

    return *this;
}

miral::SlowKeys& miral::SlowKeys::on_key_accepted(std::function<void(unsigned int)>&& on_key_accepted)
{
    auto const state = self->state.lock();
    state->on_key_accepted = std::move(on_key_accepted);
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->slow_keys().on_key_accepted(std::move(state->on_key_accepted));

    return *this;
}

void miral::SlowKeys::operator()(mir::Server& server)
{
    constexpr auto* enable_slow_keys_opt = "enable-slow-keys";
    constexpr auto* slow_keys_hold_delay = "slow-keys-hold-delay";

    {
        auto const state = self->state.lock();
        server.add_configuration_option(
            enable_slow_keys_opt,
            "Enable slow keys (presses not registering unless keys are held for a certain period)",
            state->enabled);
        server.add_configuration_option(
            slow_keys_hold_delay,
            "delay required before a press registers",
            static_cast<int>(state->hold_delay.count()));
    }

    server.add_init_callback(
        [&server, this]
        {
            auto const am = server.the_accessibility_manager();
            self->accessibility_manager = am;

            if (self->state.lock()->enabled)
                am->slow_keys_enabled(true);

            auto& sk = am->slow_keys();
            auto const state = self->state.lock();
            sk.delay(state->hold_delay);
            sk.on_key_accepted(std::move(state->on_key_accepted));
            sk.on_key_rejected(std::move(state->on_key_rejected));
            sk.on_key_down(std::move(state->on_key_down));
        });
}
