/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "basic_slow_keys_transformer.h"
#include "mir/events/input_event.h"
#include "mir/events/keyboard_event.h"
#include "mir/main_loop.h"

mir::shell::BasicSlowKeysTransformer::BasicSlowKeysTransformer(std::shared_ptr<MainLoop> const& main_loop) :
    main_loop{main_loop}
{
}

bool mir::shell::BasicSlowKeysTransformer::transform_input_event(
    mir::input::InputEventTransformer::EventDispatcher const& dispatcher,
    mir::input::EventBuilder*,
    MirEvent const& event)
{
    if (event.type() != mir_event_type_input)
        return false;

    auto* const input_event = event.to_input();
    if (input_event->input_type() != mir_input_event_type_key)
        return false;

    auto* const key_event = input_event->to_keyboard();
    auto const keysym = key_event->keysym();
    auto const kif = keys_in_flight.lock();
    switch (key_event->action())
    {
    case mir_keyboard_action_down:
        {
            auto alarm = main_loop->create_alarm(
                [this, dispatcher, event_clone = std::shared_ptr<MirEvent>(event.clone()), keysym]
                {
                    auto const kif = keys_in_flight.lock();
                    auto iter = kif->find(keysym);
                    if (iter == kif->end())
                        return;

                    auto& [_, data] = *iter;
                    data.second = true;

                    dispatcher(event_clone);
                    config.lock()->on_key_accepted(keysym);
                });

            auto const config_ = config.lock();
            alarm->reschedule_in(config_->delay);
            kif->insert_or_assign(keysym, std::pair{std::move(alarm), false});
            config_->on_key_down(keysym);

            return true;
        }
    case mir_keyboard_action_up:
        {
            auto iter = kif->find(keysym);
            if (iter == kif->end())
                return false;

            auto& [_, data] = *iter;
            auto& [alarm, dispatched] = data;

            if (dispatched)
                return false;

            alarm->cancel();
            config.lock()->on_key_rejected(keysym);

            return true;
        }

    case mir_keyboard_action_repeat:
    case mir_keyboard_action_modifiers:
    case mir_keyboard_actions:
        break;
    }

    return false;
}

void mir::shell::BasicSlowKeysTransformer::on_key_down(std::function<void(unsigned int)>&& okd)
{
    config.lock()->on_key_down = std::move(okd);
}

void mir::shell::BasicSlowKeysTransformer::on_key_rejected(std::function<void(unsigned int)>&& okr)
{
    config.lock()->on_key_rejected = std::move(okr);
}

void mir::shell::BasicSlowKeysTransformer::on_key_accepted(std::function<void(unsigned int)>&& oka)
{
    config.lock()->on_key_accepted = std::move(oka);
}

void mir::shell::BasicSlowKeysTransformer::delay(std::chrono::milliseconds delay)
{
    config.lock()->delay = delay;
}
