/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#ifndef MIR_EXAMPLES_WINDOW_MANAGEMENT_H_
#define MIR_EXAMPLES_WINDOW_MANAGEMENT_H_

#include "mir/geometry/rectangles.h"
#include "mir/input/event_filter.h"
#include "mir/shell/shell.h"

#include <memory>

///\example server_example_window_management.h
/// Demonstrate simple window management strategies

namespace mir
{
class Server;

namespace examples
{
class WindowManager :
    public virtual shell::Shell,
    public virtual input::EventFilter
{
public:
    virtual void add_display(geometry::Rectangle const& area) = 0;

    virtual void remove_display(geometry::Rectangle const& area) = 0;
};

class WindowManagmentFactory
{
public:
    explicit WindowManagmentFactory(Server& server) : server(server) {}

    auto window_manager() -> std::shared_ptr<WindowManager>;

private:
    Server& server;
    std::weak_ptr<WindowManager> wm;
};

extern char const* const wm_option;
extern char const* const wm_description;
}
}

#endif
