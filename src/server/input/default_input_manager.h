/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canonical.com>
 */

#ifndef MIR_INPUT_DEFAULT_INPUT_MANAGER_H_
#define MIR_INPUT_DEFAULT_INPUT_MANAGER_H_

#include "mir/input/input_manager.h"

#include <thread>
#include <atomic>

namespace mir
{
namespace dispatch
{
class MultiplexingDispatchable;
class ThreadedDispatcher;
class ActionQueue;
}
namespace input
{
class Platform;
class InputEventHandlerRegister;
class InputDeviceRegistry;

class DefaultInputManager : public InputManager
{
public:
    DefaultInputManager(
        std::shared_ptr<dispatch::MultiplexingDispatchable> const& multiplexer,
        std::shared_ptr<Platform> const& platform);
    ~DefaultInputManager();

    void start() override;
    void stop() override;
    void pause_for_config() override;
    void continue_after_config() override;
private:
    void start_platforms();
    void stop_platforms();
    std::shared_ptr<Platform> const platform;
    std::shared_ptr<dispatch::MultiplexingDispatchable> const multiplexer;
    std::shared_ptr<dispatch::ActionQueue> const queue;
    std::unique_ptr<dispatch::ThreadedDispatcher> input_thread;

    enum class State
    {
        started,
        stopped,
        starting,
        stopping
    };
    std::atomic<State> state;
};

}
}
#endif
