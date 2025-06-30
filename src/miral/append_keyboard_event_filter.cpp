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

#include "miral/append_keyboard_event_filter.h"

#include <mir/input/event_filter.h>
#include <mir/input/composite_event_filter.h>
#include <mir/server.h>

class miral::AppendKeyboardEventFilter::Filter : public mir::input::EventFilter
{
public:
    Filter(std::function<bool(MirKeyboardEvent const* event)> const& filter) :
        filter{filter} {}

    bool handle(MirEvent const& event) override
    {
        // Skip non-input events
        if (mir_event_get_type(&event) != mir_event_type_input)
            return false;

        // Skip non-key input events
        MirInputEvent const* input_event = mir_event_get_input_event(&event);
        if (mir_input_event_get_type(input_event) != mir_input_event_type_key)
            return false;

        MirKeyboardEvent const* kev = mir_input_event_get_keyboard_event(input_event);
        return filter(kev);
    }

private:
    std::function<bool(MirKeyboardEvent const* event)> const filter;
};

miral::AppendKeyboardEventFilter::AppendKeyboardEventFilter(std::function<bool(MirKeyboardEvent const* event)> const& filter) :
    filter{std::make_shared<Filter>(filter)}
{
}

void miral::AppendKeyboardEventFilter::operator()(mir::Server& server)
{
    server.add_init_callback([this, &server] { server.the_composite_event_filter()->append(filter); });
}
