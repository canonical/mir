/*
 * Copyright © 2016 Canonical Ltd.
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
 *
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "miral/append_event_filter.h"

#include <mir/input/event_filter.h>
#include <mir/input/composite_event_filter.h>
#include <mir/server.h>

class miral::AppendEventFilter::Filter : public mir::input::EventFilter
{
public:
    Filter(std::function<int(MirEvent const* event)> const& filter) :
        filter{filter} {}

    bool handle(MirEvent const& event) override
    {
        return filter(&event);
    }

private:
    std::function<int(MirEvent const* event)> const filter;
};

miral::AppendEventFilter::AppendEventFilter(std::function<int(MirEvent const* event)> const& filter) :
    filter{std::make_shared<Filter>(filter)}
{
}

void miral::AppendEventFilter::operator()(mir::Server& server)
{
    server.add_init_callback([this, &server] { server.the_composite_event_filter()->append(filter); });
}
