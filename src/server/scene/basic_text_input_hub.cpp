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

#include "basic_text_input_hub.h"

namespace ms = mir::scene;

auto ms::BasicTextInputHub::set_handler_state(
    std::shared_ptr<TextInputChangeHandler> const& handler,
    bool new_input_field,
    TextInputState const& state) -> TextInputStateSerial
{
    std::unique_lock lock{mutex};
    if (active_handler != handler)
    {
        new_input_field = true;
        active_handler = handler;
    }
    current_state = state;
    current_serial = TextInputStateSerial{current_serial.as_value() + 1};
    auto const serial = current_serial;
    lock.unlock();
    multiplexer.activated(serial, new_input_field, state);
    return serial;
}

void ms::BasicTextInputHub::deactivate_handler(std::shared_ptr<TextInputChangeHandler> const& handler)
{
    std::unique_lock lock{mutex};
    if (active_handler != handler)
    {
        return;
    }
    active_handler = {};
    current_state = std::nullopt;
    lock.unlock();
    multiplexer.deactivated();
}

void ms::BasicTextInputHub::text_changed(TextInputChange const& change)
{
    std::unique_lock lock{mutex};
    auto const local_handler = active_handler;
    lock.unlock();
    if (local_handler)
    {
        local_handler->text_changed(change);
    }
}

void ms::BasicTextInputHub::send_initial_state(std::weak_ptr<TextInputStateObserver> const& observer)
{
    std::unique_lock lock{mutex};
    auto const state = current_state;
    auto const serial = current_serial;
    lock.unlock();
    if (auto const target = observer.lock())
    {
        if (state)
        {
            target->activated(serial, true, *state);
        }
        else
        {
            target->deactivated();
        }
    }
}

void ms::BasicTextInputHub::show_input_panel()
{
    multiplexer.show_input_panel();
}

void ms::BasicTextInputHub::hide_input_panel()
{
    multiplexer.hide_input_panel();
}
