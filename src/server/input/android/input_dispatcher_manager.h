/*
 * Copyright © 2013 Canonical Ltd.
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


#ifndef MIR_INPUT_ANDROID_INPUT_DISPATCHER_MANAGER_H_
#define MIR_INPUT_ANDROID_INPUT_DISPATCHER_MANAGER_H_

#include "mir/input/input_manager.h"

#include <utils/StrongPointer.h>

namespace android
{
class InputDispatcherInterface;
}

namespace droidinput = android;

namespace mir
{
namespace input
{
namespace android
{
class InputThread;

class InputDispatcherManager : public mir::input::InputManager
{
public:
    InputDispatcherManager(
        droidinput::sp<droidinput::InputDispatcherInterface> const& dispatcher,
        std::shared_ptr<InputThread> const& dispatcher_thread);
    ~InputDispatcherManager();

    void start();
    void stop();

    std::shared_ptr<InputChannel> make_input_channel();

private:
    droidinput::sp<droidinput::InputDispatcherInterface> const dispatcher;
    std::shared_ptr<InputThread> const dispatcher_thread;
};
}
}
}

#endif /* MIR_INPUT_ANDROID_INPUT_DISPATCHER_MANAGER_H_ */
