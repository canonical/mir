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

#include "miral/bounce_keys.h"

#include "mir/events/event.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/event_filter.h"
#include "mir/input/mousekeys_keymap.h"
#include "mir/main_loop.h"
#include "mir/options/option.h"
#include "mir/server.h"
#include "mir/synchronised.h"
#include "mir/time/alarm.h"

#include <chrono>
#include <optional>

struct miral::BounceKeys::Self
{
    struct BounceKeysFilter : public mir::input::EventFilter
    {
        struct State
        {
            // Runtime state
            std::optional<mir::input::XkbSymkey> last_key;

            // Configuration state
            std::function<void(unsigned int keysym)> on_press_rejected{[](auto) {}};
            std::function<void()> on_enabled{[]{}}, on_disabled{[]{}};
            std::chrono::milliseconds delay{500};
        };

        BounceKeysFilter(
            std::shared_ptr<mir::MainLoop> const& main_loop, std::shared_ptr<mir::Synchronised<State>> const& state) :
            state{state},
            last_key_clearer{main_loop->create_alarm([this] { this->state->lock()->last_key.reset(); })}
        {
        }

        bool handle(MirEvent const& event) override
        {
            if(event.type() != mir_event_type_input)
                return false;

            auto const* input_event = event.to_input();
            if(input_event->input_type() != mir_input_event_type_key)
                return false;

            auto const* key_event = input_event->to_keyboard();

            // Only consume "down" events
            auto const action = key_event->action();
            if (action == mir_keyboard_action_up || action == mir_keyboard_action_repeat)
                return false;

            auto const keysym = mir_keyboard_event_keysym(key_event);

            auto state_ = state->lock();

            last_key_clearer->cancel();
            last_key_clearer->reschedule_in(state_->delay);

            if(!state_->last_key || *state_->last_key != keysym)
            {
                state_->last_key = keysym;
                return false;
            }

            state_->on_press_rejected(keysym);

            return true;
        }

        std::shared_ptr<mir::Synchronised<State>> state;
        std::unique_ptr<mir::time::Alarm> const last_key_clearer;
    };

    Self(bool enabled_by_default) :
        enable_on_startup{enabled_by_default},
        filter_state{std::make_shared<mir::Synchronised<BounceKeysFilter::State>>()}
    {
    }

    bool enable_on_startup{false};

    std::weak_ptr<mir::MainLoop> main_loop;
    std::weak_ptr<mir::input::CompositeEventFilter> composite_event_filter;

    std::shared_ptr<BounceKeysFilter> bounce_keys_filter;
    std::shared_ptr<mir::Synchronised<BounceKeysFilter::State>> filter_state;
};

miral::BounceKeys::BounceKeys(bool enable_by_default)
    : self{std::make_shared<Self>(enable_by_default)}
{
}

void miral::BounceKeys::operator()(mir::Server& server)
{
    constexpr static auto* enable_bounce_keys_opt = "enable-bounce-keys";
    constexpr static auto* bounce_keys_delay_opt = "bounce-keys-delay";

    server.add_configuration_option(enable_bounce_keys_opt, "Enable bounce keys", self->enable_on_startup);
    server.add_configuration_option(
        bounce_keys_delay_opt,
        "The minimum duration allowed between key presses. Presses occuring sooner than this duration will not be "
        "propagated to applications",
        static_cast<int>(self->filter_state->lock()->delay.count()));

    server.add_init_callback(
        [&server, this]
        {
            self->main_loop = server.the_main_loop();
            self->composite_event_filter = server.the_composite_event_filter();

            auto const options = server.get_options();
            delay(std::chrono::milliseconds{options->get<int>(bounce_keys_delay_opt)});

            if (options->get<bool>(enable_bounce_keys_opt))
                enable();
        });
}

miral::BounceKeys& miral::BounceKeys::delay(std::chrono::milliseconds delay)
{
    self->filter_state->lock()->delay = delay;
    return *this;
}

miral::BounceKeys& miral::BounceKeys::on_press_rejected(std::function<void(unsigned int keysym)>&& on_press_rejected)
{
    self->filter_state->lock()->on_press_rejected = std::move(on_press_rejected);
    return *this;
}

miral::BounceKeys& miral::BounceKeys::on_enabled(std::function<void()>&& on_enabled)
{
    self->filter_state->lock()->on_enabled = std::move(on_enabled);
    return *this;
}

miral::BounceKeys& miral::BounceKeys::on_disabled(std::function<void()>&& on_disabled)
{
    self->filter_state->lock()->on_disabled = std::move(on_disabled);
    return *this;
}

miral::BounceKeys& miral::BounceKeys::enable() 
{
    self->enable_on_startup = true;
    if (self->bounce_keys_filter)
        return *this;

    if (auto [main_loop, composite_event_filter] =
            std::pair{self->main_loop.lock(), self->composite_event_filter.lock()};
        main_loop && composite_event_filter)
    {
        self->bounce_keys_filter = std::make_shared<Self::BounceKeysFilter>(main_loop, self->filter_state);
        composite_event_filter->prepend(self->bounce_keys_filter);
        self->filter_state->lock()->on_enabled();
    }

    return *this;
}

miral::BounceKeys& miral::BounceKeys::disable()
{
    self->enable_on_startup = false;
    if (!self->bounce_keys_filter)
        return *this;

    self->bounce_keys_filter.reset();
    self->filter_state->lock()->on_disabled();

    return *this;
}
