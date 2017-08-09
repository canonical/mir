/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef EXAMPLE_INPUT_EVENT_FILTER_H_
#define EXAMPLE_INPUT_EVENT_FILTER_H_

#include "mir/input/event_filter.h"

#include <functional>
#include <memory>

namespace mir
{
class Server;

namespace examples
{
class QuitFilter : public mir::input::EventFilter
{
public:
    QuitFilter(std::function<void()> const& quit_action);

    bool handle(MirEvent const& event) override;

private:
    std::function<void()> const quit_action;
};

// Set up a Ctrl+Alt+BkSp => quit
auto make_quit_filter_for(Server& server) -> std::shared_ptr<mir::input::EventFilter>;
}
}

#endif /* EXAMPLE_INPUT_EVENT_FILTER_H_ */
