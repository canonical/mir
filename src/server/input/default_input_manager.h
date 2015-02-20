/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_DEFAULT_INPUT_MANAGER_H_
#define MIR_INPUT_DEFAULT_INPUT_MANAGER_H_

#include "main_loop_wrapper.h"

#include "mir/input/input_manager.h"
#include "mir/library_shared_ptr.h"

#include <thread>
#include <mutex>
#include <atomic>

namespace mir
{
class MainLoop;
namespace input
{
class Platform;
class InputEventHandlerRegister;
class InputDeviceRegistry;

class DefaultInputManager : public InputManager
{
public:
    DefaultInputManager(std::shared_ptr<Platform> const& input_platform,
                        std::shared_ptr<InputDeviceRegistry> const& registry,
                        std::shared_ptr<MainLoop> const& input_event_loop);
    ~DefaultInputManager();
    void start() override;
    void stop() override;
private:
    std::shared_ptr<Platform> const input_platform;
    std::shared_ptr<MainLoop> const input_event_loop;
    std::shared_ptr<InputDeviceRegistry> const input_device_registry;
    MainLoopWrapper input_handler_register;

    enum class State
    {
        starting, running, stopping, stopped
    };

    std::atomic<State> thread_state;

    std::thread input_thread;
};

}
}
#endif
