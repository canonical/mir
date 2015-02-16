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

#ifndef MIR_EXAMPLES_SHELL_H_
#define MIR_EXAMPLES_SHELL_H_

#include "mir/input/event_filter.h"
#include "mir/shell/shell.h"

#include <memory>

///\example server_example_shell.h
/// Demonstrate a shell supporting selection of a window manager

namespace mir
{
class Server;
namespace geometry { class Rectangle; }

namespace examples
{
/// For window management we need a shell that tracks input events and display changes
class Shell :
    public virtual shell::Shell,
    public virtual input::EventFilter
{
public:
    virtual void add_display(geometry::Rectangle const& area) = 0;

    virtual void remove_display(geometry::Rectangle const& area) = 0;
};

/// We need to hook into several places, a caching factory is simplifies this
class ShellFactory
{
public:
    explicit ShellFactory(Server& server) : server(server) {}

    auto shell() -> std::shared_ptr<Shell>;

private:
    Server& server;
    std::weak_ptr<Shell> shell_;
};

extern char const* const wm_option;
extern char const* const wm_description;
}
}

#endif
