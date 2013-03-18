/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Daniel d'Andradra <daniel.dandrada@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_INPUT_MANAGER_H_
#define MIR_INPUT_ANDROID_INPUT_MANAGER_H_

#include "mir/input/input_manager.h"

#include <utils/StrongPointer.h>

#include <initializer_list>

namespace android
{
class EventHubInterface;
class InputDispatcherInterface;
}

namespace droidinput = android;

namespace mir
{
namespace graphics
{
class ViewableArea;
}
namespace input
{
class CursorListener;

namespace android
{
class InputThread;
class InputConfiguration;

class InputManager : public mir::input::InputManager
{
public:
    explicit InputManager(
        std::shared_ptr<InputConfiguration> const& input_configuration);
    virtual ~InputManager();

    void start();
    void stop();
    
    std::shared_ptr<InputChannel> make_input_channel();
    

protected:
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

private:
    droidinput::sp<droidinput::EventHubInterface> event_hub;
    droidinput::sp<droidinput::InputDispatcherInterface> dispatcher;

    std::shared_ptr<InputThread> reader_thread;
    std::shared_ptr<InputThread> dispatcher_thread;
};

}
}
}

#endif // MIR_INPUT_INPUT_MANAGER
