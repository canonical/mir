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

#ifndef MIR_SCENE_BASIC_TEXT_INPUT_HUB_H_
#define MIR_SCENE_BASIC_TEXT_INPUT_HUB_H_

#include "mir/scene/text_input_hub.h"
#include "mir/observer_multiplexer.h"

#include <mutex>

namespace mir
{
namespace scene
{
class TextInputStateObserverMultiplexer: public ObserverMultiplexer<TextInputStateObserver>
{
public:
    TextInputStateObserverMultiplexer()
        : ObserverMultiplexer{immediate_executor}
    {
    }

    void activated(TextInputStateSerial serial, bool new_input_field, TextInputState const& state) override
    {
        for_each_observer(&TextInputStateObserver::activated, serial, new_input_field, state);
    }

    void deactivated() override
    {
        for_each_observer(&TextInputStateObserver::deactivated);
    }

    void show_input_panel() override
    {
        for_each_observer(&TextInputStateObserver::show_input_panel);
    }

    void hide_input_panel() override
    {
        for_each_observer(&TextInputStateObserver::hide_input_panel);
    }
};

class BasicTextInputHub : public TextInputHub
{
public:
    auto set_handler_state(
        std::shared_ptr<TextInputChangeHandler> const& handler,
        bool new_input_field,
        TextInputState const& state) -> TextInputStateSerial override;

    void deactivate_handler(std::shared_ptr<TextInputChangeHandler> const& handler) override;

    void text_changed(TextInputChange const& change) override;

    /// Implement ObserverRegistrar<TextInputStateObserver>
    /// @{
    void register_interest(std::weak_ptr<TextInputStateObserver> const& observer) override
    {
        multiplexer.register_interest(observer);
        send_initial_state(observer);
    }
    void register_interest(std::weak_ptr<TextInputStateObserver> const& observer, Executor& executor) override
    {
        multiplexer.register_interest(observer, executor);
        send_initial_state(observer);
    }
    void register_early_observer(
        std::weak_ptr<TextInputStateObserver> const& observer,
        Executor& executor) override
    {
        multiplexer.register_early_observer(observer, executor);
        send_initial_state(observer);
    }
    void unregister_interest(TextInputStateObserver const& observer) override
    {
        multiplexer.unregister_interest(observer);
    }
    /// @}

    void show_input_panel() override;
    void hide_input_panel() override;

private:
    TextInputStateObserverMultiplexer multiplexer;

    void send_initial_state(std::weak_ptr<TextInputStateObserver> const& observer);

    std::mutex mutex;
    std::shared_ptr<TextInputChangeHandler> active_handler;
    TextInputStateSerial current_serial{0};
    std::optional<TextInputState> current_state;
};
}
}

#endif // MIR_SCENE_BASIC_TEXT_INPUT_HUB_H_
