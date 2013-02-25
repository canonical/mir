/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
#ifndef MIR_SERVER_CONFIGURATION_H_
#define MIR_SERVER_CONFIGURATION_H_

#include <memory>

namespace mir
{
namespace compositor
{
class Drawer;
}
namespace frontend
{
class Communicator;
}

namespace sessions
{
class SessionStore;
}
namespace graphics
{
class Display;
}
namespace input
{
class InputManager;
class EventFilter;
}

class ServerConfiguration
{
public:
    virtual std::shared_ptr<frontend::Communicator> the_communicator() = 0;
    virtual std::shared_ptr<sessions::SessionStore> the_session_store() = 0;
    virtual std::shared_ptr<graphics::Display> the_display() = 0;
    virtual std::shared_ptr<compositor::Drawer> the_drawer() = 0;

    // TODO this should not be taking a parameter, but as
    // TODO input still needs proper integration left for later
    virtual std::shared_ptr<input::InputManager> the_input_manager(
        const std::initializer_list<std::shared_ptr<input::EventFilter> const>& event_filters) = 0;

protected:
    ServerConfiguration() = default;
    virtual ~ServerConfiguration() {}

    ServerConfiguration(ServerConfiguration const&) = delete;
    ServerConfiguration& operator=(ServerConfiguration const&) = delete;
};
}


#endif /* MIR_SERVER_CONFIGURATION_H_ */
