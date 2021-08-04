/*
 * Copyright © 2021 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
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
class TextInputStateObserverMultiplexer: mir::Executor, public ObserverMultiplexer<TextInputStateObserver>
{
public:
    TextInputStateObserverMultiplexer()
        : ObserverMultiplexer{static_cast<Executor&>(*this)}
    {
    }

    // By default, dispatch events immediately. The user can override this behavior by setting their own executor.
    void spawn(std::function<void()>&& work) override
    {
        work();
    }

    void activated(bool new_input_field, TextInputState const& state) override
    {
        for_each_observer(&TextInputStateObserver::activated, new_input_field, state);
    }

    void deactivated() override
    {
        for_each_observer(&TextInputStateObserver::deactivated);
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

    void send_change(TextInputChange const& change) override;

    /// Implement ObserverRegistrar<TextInputStateObserver>
    /// @{
    void register_interest(std::weak_ptr<TextInputStateObserver> const& observer) override
    {
        multiplexer.register_interest(observer);
    }
    void register_interest(std::weak_ptr<TextInputStateObserver> const& observer, Executor& executor) override
    {
        multiplexer.register_interest(observer, executor);
    }
    void unregister_interest(TextInputStateObserver const& observer) override
    {
        multiplexer.unregister_interest(observer);
    }
    /// @}

private:
    TextInputStateObserverMultiplexer multiplexer;

    std::mutex mutex;
    std::shared_ptr<TextInputChangeHandler> active_handler;
    TextInputStateSerial last_serial{0};
};
}
}

#endif // MIR_SCENE_BASIC_TEXT_INPUT_HUB_H_
