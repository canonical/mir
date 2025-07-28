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
#include "miral/live_config.h"

#include <mir/log.h>

struct miral::HoverClick::Self
{
    explicit Self(bool enabled_by_default) :
        state{State{enabled_by_default}}
    {
    }

    struct State
    {
        explicit State(bool enabled) :
            enabled{enabled}
        {
        }

        bool enabled;

        std::chrono::milliseconds hover_duration{1000};
        int cancel_displacement_threshold{10};
        int reclick_displacement_threshold{5};
        std::function<void()> on_hover_start{[] {}};
        std::function<void()> on_hover_cancel{[] {}};
        std::function<void()> on_click_dispatched{[] {}};
    };

    mir::Synchronised<State> state;
    std::weak_ptr<mir::shell::AccessibilityManager> accessibility_manager;
};

miral::HoverClick::HoverClick(miral::live_config::Store& config_store)
    : self{std::make_shared<Self>(false)}
{
    config_store.add_bool_attribute(
        {"hover_click", "enable"},
        "Whether hover click is disabled or not",
        [this](live_config::Key const&, std::optional<bool> val)
        {
            if (!val)
                return;

            if (*val)
                enable();
            else
                disable();
        });

    auto const add_int_attribute =
        [&config_store](
            std::string_view option_name, std::string_view option_description, std::function<void(int)> on_valid_option)
    {
        config_store.add_int_attribute(
            {"hover_click", option_name},
            option_description,
            [ovo = std::move(on_valid_option)](live_config::Key const& key, std::optional<int> val)
            {
                if (!val)
                    return;

                if (*val < 0)
                    mir::log_warning(
                        "Config value %s does not support negative values. Ignoring the supplied value (%d)...",
                        key.to_string().c_str(),
                        *val);

                ovo(*val);
            });
    };

    add_int_attribute(
        "hover_duration",
        "How long in milliseconds the pointer has to stay still to dispatch a left click",
        [this](int val) { hover_duration(std::chrono::milliseconds{val}); });

    add_int_attribute(
        "cancel_displacement",
        "The distance in pixels the pointer has to move from the initial hover click position to cancel it.",
        [this](int val) { cancel_displacement_threshold(val); });

    add_int_attribute(
        "reclick_displacement",
        "the distance in pixels the pointer has to move from the the last hover click or hover click cancel position "
        "to initiate a new hover click ",
        [this](int val) { reclick_displacement_threshold(val); });
}

miral::HoverClick::HoverClick(std::shared_ptr<Self> self)
    : self{std::move(self)}
{
}

miral::HoverClick miral::HoverClick::enabled()
{
    return HoverClick{std::make_shared<Self>(true)};
}

miral::HoverClick miral::HoverClick::disabled()
{
    return HoverClick{std::make_shared<Self>(false)};
}

miral::HoverClick& miral::HoverClick::enable()
{
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click_enabled(true);
    else
        self->state.lock()->enabled = true;

    return *this;
}

miral::HoverClick& miral::HoverClick::disable()
{
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click_enabled(false);
    else
        self->state.lock()->enabled = false;

    return *this;
}

miral::HoverClick& miral::HoverClick::hover_duration(std::chrono::milliseconds hover_duration)
{
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().hover_duration(hover_duration);
    else
        self->state.lock()->hover_duration = hover_duration;

    return *this;
}

miral::HoverClick& miral::HoverClick::cancel_displacement_threshold(int displacement)
{
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().cancel_displacement_threshold(displacement);
    else
        self->state.lock()->cancel_displacement_threshold = displacement;

    return *this;
}

miral::HoverClick& miral::HoverClick::reclick_displacement_threshold(int displacement)
{
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().reclick_displacement_threshold(displacement);
    else
        self->state.lock()->reclick_displacement_threshold = displacement;

    return *this;
}

miral::HoverClick& miral::HoverClick::on_hover_start(std::function<void()> && on_hover_start)
{
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().on_hover_start(std::move(on_hover_start));
    else
        self->state.lock()->on_hover_start = std::move(on_hover_start);


    return *this;
}

miral::HoverClick& miral::HoverClick::on_hover_cancel(std::function<void()>&& on_hover_cancel)
{
    if (auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().on_hover_cancel(std::move(on_hover_cancel));
    else
        self->state.lock()->on_hover_cancel = std::move(on_hover_cancel);

    return *this;
}

miral::HoverClick& miral::HoverClick::on_click_dispatched(std::function<void()> && on_click_dispatched)
{
    if(auto const accessibility_manager = self->accessibility_manager.lock())
        accessibility_manager->hover_click().on_click_dispatched(std::move(on_click_dispatched));
    else
        self->state.lock()->on_click_dispatched = std::move(on_click_dispatched);

    return *this;
}

void miral::HoverClick::operator()(mir::Server& server)
{
    server.add_init_callback(
        [&server, this]
        {
            self->accessibility_manager = server.the_accessibility_manager();
            if(auto accessibility_manager = server.the_accessibility_manager())
            {
                auto state = self->state.lock();

                if (state->enabled)
                    accessibility_manager->hover_click_enabled(true);

                auto& hc = accessibility_manager->hover_click();
                hc.hover_duration(state->hover_duration);
                hc.cancel_displacement_threshold(state->cancel_displacement_threshold);
                hc.reclick_displacement_threshold(state->reclick_displacement_threshold);
                hc.on_hover_start(std::move(state->on_hover_start));
                hc.on_hover_cancel(std::move(state->on_hover_cancel));
                hc.on_click_dispatched(std::move(state->on_click_dispatched));
            }
        });
}
