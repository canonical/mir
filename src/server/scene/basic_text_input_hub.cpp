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

#include "basic_text_input_hub.h"

namespace ms = mir::scene;

auto ms::BasicTextInputHub::set_handler_state(
    std::shared_ptr<TextInputChangeHandler> const& handler,
    bool new_input_field,
    TextInputState const& state) -> TextInputStateSerial
{
    std::unique_lock<std::mutex> lock{mutex};
    if (active_handler != handler)
    {
        new_input_field = true;
        active_handler = handler;
    }
    last_serial = TextInputStateSerial{last_serial.as_value() + 1};
    auto const serial = last_serial;
    lock.unlock();
    multiplexer.activated(serial, new_input_field, state);
    return serial;
}

void ms::BasicTextInputHub::deactivate_handler(std::shared_ptr<TextInputChangeHandler> const& handler)
{
    std::unique_lock<std::mutex> lock{mutex};
    if (active_handler != handler)
    {
        return;
    }
    active_handler = {};
    lock.unlock();
    multiplexer.deactivated();
}

void ms::BasicTextInputHub::send_change(TextInputChange const& change)
{
    std::unique_lock<std::mutex> lock{mutex};
    auto const local_handler = active_handler;
    lock.unlock();
    if (local_handler)
    {
        local_handler->change(change);
    }
}
