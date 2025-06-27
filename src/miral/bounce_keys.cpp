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

#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/input/composite_event_filter.h"
#include "mir/input/event_filter.h"
#include "mir/input/mousekeys_keymap.h"
#include "mir/log.h"
#include "mir/main_loop.h"
#include "mir/server.h"
#include "mir/synchronised.h"
#include "mir/time/alarm.h"

#include <chrono>
#include <optional>

namespace mi = mir::input;

struct miral::BounceKeys::Self
{
    struct Config
    {
        explicit Config(bool enabled_by_default) :
            enabled{enabled_by_default}
        {
        }

        bool enabled;
        std::chrono::milliseconds delay{200};
        std::function<void(unsigned int keysym)> on_press_rejected{[](auto) {}};
    };

    struct BounceKeysFilter : public mi::EventFilter
    {
        struct RuntimeState
        {
            std::optional<mi::XkbSymkey> last_key;
        };

        BounceKeysFilter(
            std::shared_ptr<mir::MainLoop> const& main_loop, std::shared_ptr<mir::Synchronised<Config>> config) :
            config{std::move(config)},
            last_key_clearer{main_loop->create_alarm([this] { this->runtime_state.lock()->last_key.reset(); })}
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
            if (action != mir_keyboard_action_down)
                return false;

            auto const keysym = mir_keyboard_event_keysym(key_event);

            auto state = runtime_state.lock();
            auto config_ = config->lock();

            last_key_clearer->cancel();
            last_key_clearer->reschedule_in(config_->delay);

            if(!state->last_key || *state->last_key != keysym)
            {
                state->last_key = keysym;
                return false;
            }

            config_->on_press_rejected(keysym);

            return true;
        }

        mir::Synchronised<RuntimeState> runtime_state;
        std::shared_ptr<mir::Synchronised<Config>> const config;
        std::unique_ptr<mir::time::Alarm> const last_key_clearer;
    };

    void enable()
    {
        if (auto [ml, cef] = std::pair{main_loop.lock(), composite_event_filter.lock()}; ml && cef)
        {
            bounce_keys_filter = std::make_shared<Self::BounceKeysFilter>(ml, config);
            cef->prepend(bounce_keys_filter);
        }
    }

    explicit Self(bool enabled_by_default) :
        config{std::make_shared<mir::Synchronised<Config>>(Config{enabled_by_default})}
    {
    }

    std::shared_ptr<mir::Synchronised<Config>> const config;
    std::shared_ptr<BounceKeysFilter> bounce_keys_filter;

    std::weak_ptr<mir::MainLoop> main_loop;
    std::weak_ptr<mi::CompositeEventFilter> composite_event_filter;
};

miral::BounceKeys::BounceKeys(bool enable_by_default)
    : self{std::make_shared<Self>(enable_by_default)}
{
}

void miral::BounceKeys::operator()(mir::Server& server)
{
    server.add_init_callback(
        [&server, this]
        {
            self->main_loop = server.the_main_loop();
            self->composite_event_filter = server.the_composite_event_filter();

            if (self->config->lock()->enabled)
                self->enable();
        });
}

miral::BounceKeys& miral::BounceKeys::delay(std::chrono::milliseconds delay)
{
    if(delay < std::chrono::milliseconds{0})
        mir::log_warning("Bounce keys delay set to a negative value. Clamping to zero.");

    self->config->lock()->delay = std::max(delay, std::chrono::milliseconds{0});
    return *this;
}

miral::BounceKeys& miral::BounceKeys::on_press_rejected(std::function<void(unsigned int keysym)>&& on_press_rejected)
{
    self->config->lock()->on_press_rejected = std::move(on_press_rejected);
    return *this;
}

miral::BounceKeys& miral::BounceKeys::enable() 
{
    auto const config = self->config->lock();
    if (config->enabled)
        return *this;
    config->enabled = true;

    self->enable();

    return *this;
}

miral::BounceKeys& miral::BounceKeys::disable()
{
    auto const config = self->config->lock();
    if (!config->enabled)
        return *this;
    config->enabled = false;

    self->bounce_keys_filter.reset();

    return *this;
}
