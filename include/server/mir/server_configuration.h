/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */
#ifndef MIR_SERVER_CONFIGURATION_H_
#define MIR_SERVER_CONFIGURATION_H_

#include <memory>

namespace mir
{
namespace compositor
{
class Compositor;
}
namespace frontend
{
class Communicator;
class Shell;
}
namespace shell
{
class SessionContainer;
}
namespace graphics
{
class Display;
class DisplayConfigurationPolicy;
class Platform;
}
namespace input
{
class InputManager;
class EventFilter;
class InputConfiguration;
}

class MainLoop;
class DisplayChanger;

class ServerConfiguration
{
public:
    // TODO most of these interfaces are wider DisplayServer needs...
    // TODO ...some or all of them need narrowing
    virtual std::shared_ptr<frontend::Communicator> the_communicator() = 0;
    virtual std::shared_ptr<graphics::Display> the_display() = 0;
    virtual std::shared_ptr<compositor::Compositor> the_compositor() = 0;
    virtual std::shared_ptr<input::InputManager> the_input_manager() = 0;
    virtual std::shared_ptr<MainLoop> the_main_loop() = 0;
    virtual std::shared_ptr<DisplayChanger> the_display_changer() = 0;
    virtual std::shared_ptr<graphics::Platform>  the_graphics_platform() = 0;
    virtual std::shared_ptr<input::InputConfiguration> the_input_configuration() = 0;

protected:
    ServerConfiguration() = default;
    virtual ~ServerConfiguration() { /* TODO: make nothrow */ }

    ServerConfiguration(ServerConfiguration const&) = delete;
    ServerConfiguration& operator=(ServerConfiguration const&) = delete;
};
}


#endif /* MIR_SERVER_CONFIGURATION_H_ */
