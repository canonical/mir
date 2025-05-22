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

#include "miral/hover_click.h"

#include "mir/options/option.h"
#include "mir/server.h"
#include "mir/shell/accessibility_manager.h"
#include "mir/shell/hover_click_transformer.h"
#include "mir/synchronised.h"

struct miral::HoverClick::Self
{
    Self(bool enabled_by_default) :
        state{enabled_by_default}
    {
    }

    struct State
    {
        State(bool enabled) :
            enabled{enabled}
        {
        }

        bool enabled;

        std::chrono::milliseconds hover_duration{1000};
        float cancel_displacement_threshold{10};
        std::function<void()> on_enabled{[] {}};
        std::function<void()> on_disabled{[] {}};
        std::function<void()> on_hover_start{[] {}};
        std::function<void()> on_hover_cancel{[] {}};
        std::function<void()> on_click_dispatched{[] {}};
    };

    mir::Synchronised<State> state;
    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
};

miral::HoverClick::HoverClick(bool enabled_by_default)
    : self{std::make_shared<Self>(enabled_by_default)}
{
}

miral::HoverClick& miral::HoverClick::enable()
{
    self->state.lock()->enabled = true;
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click_enabled(true);

    return *this;
}

miral::HoverClick& miral::HoverClick::disable()
{
    self->state.lock()->enabled = false;
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click_enabled(false);

    return *this;
}

miral::HoverClick& miral::HoverClick::hover_duration(std::chrono::milliseconds hover_duration)
{
    self->state.lock()->hover_duration = hover_duration;
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().hover_duration(hover_duration);

    return *this;
}

miral::HoverClick& miral::HoverClick::cancel_displacement_threshold(float displacement)
{
    self->state.lock()->cancel_displacement_threshold = displacement;
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().cancel_displacement_threshold(displacement);

    return *this;
}

miral::HoverClick& miral::HoverClick::on_enabled(std::function<void()> && on_enabled)
{
    auto const state = self->state.lock();
    state->on_enabled = std::move(on_enabled);
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().on_enabled(std::move(state->on_enabled));

    return *this;
}

miral::HoverClick& miral::HoverClick::on_disabled(std::function<void()> && on_disabled)
{
    auto const state = self->state.lock();
    state->on_disabled = std::move(on_disabled);
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().on_disabled(std::move(state->on_disabled));

    return *this;
}

miral::HoverClick& miral::HoverClick::on_hover_start(std::function<void()> && on_hover_start)
{
    auto const state = self->state.lock();
    state->on_hover_start = std::move(on_hover_start);
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().on_hover_start(std::move(state->on_hover_start));

    return *this;
}

miral::HoverClick& miral::HoverClick::on_hover_cancel(std::function<void()> && on_hover_cancel)
{
    auto const state = self->state.lock();
    state->on_hover_cancel = std::move(on_hover_cancel);
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().on_hover_cancel(std::move(state->on_hover_cancel));

    return *this;
}

miral::HoverClick& miral::HoverClick::on_click_dispatched(std::function<void()> && on_click_dispatched)
{
    auto const state = self->state.lock();
    state->on_click_dispatched = std::move(on_click_dispatched);
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().on_click_dispatched(std::move(state->on_click_dispatched));

    return *this;
}

void miral::HoverClick::operator()(mir::Server& server)
{
    constexpr auto* enable_hover_click_opt = "enable-hover-click";
    constexpr auto* hover_click_duration_opt = "hover-click-duration";
    constexpr auto* hover_click_cancel_displacement_threshold = "hover-click-cancel-displacement-threshold";

    {
        auto const state = self->state.lock();
        server.add_configuration_option(
            enable_hover_click_opt,
            "Enable hover click (hovering the pointer to simulate a click)",
            state->enabled);
        server.add_configuration_option(
            hover_click_duration_opt,
            "How long the pointer has to stay still to dispatch a left click in milliseconds",
            static_cast<int>(state->hover_duration.count()));
        server.add_configuration_option(
            hover_click_cancel_displacement_threshold,
            "How far the pointer is allowed to move from the original hover click position before its cancelled in "
            "pixels",
            state->cancel_displacement_threshold);
    }

    server.add_init_callback(
        [&server, this]
        {
            self->accessibility_manager = server.the_accessibility_manager();
            auto const options = server.get_options();

            hover_duration(std::chrono::milliseconds{options->get<int>(hover_click_duration_opt)});
            cancel_displacement_threshold(options->get<double>(hover_click_cancel_displacement_threshold));

            if(auto& on_enabled_ = self->state.lock()->on_enabled)
                on_enabled(std::move(on_enabled_));
            if(auto& on_disabled_ = self->state.lock()->on_disabled)
                on_disabled(std::move(on_disabled_));
            if(auto& on_hover_start_ = self->state.lock()->on_hover_start)
                on_hover_start(std::move(on_hover_start_));
            if(auto& on_hover_cancel_ = self->state.lock()->on_hover_cancel)
                on_hover_cancel(std::move(on_hover_cancel_));
            if(auto& on_click_dispatched_ = self->state.lock()->on_click_dispatched)
                on_click_dispatched(std::move(on_click_dispatched_));

            if(options->get<bool>(enable_hover_click_opt))
                enable();
        });
}
