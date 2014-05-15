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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Daniel d'Andradra <daniel.dandrada@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_INPUT_MANAGER_H_
#define MIR_INPUT_ANDROID_INPUT_MANAGER_H_

#include "mir/input/input_manager.h"

#include <memory>

namespace android
{
class EventHubInterface;
}

namespace droidinput = android;

namespace mir
{

namespace input
{
class InputDispatcher;
namespace android
{
class InputThread;

/// Encapsulates the instances of the Android input stack that might require startup and
//  shutdown calls, that is to say an EventHub tied to an InputReader tied to an
//  InputDispatcher.
class InputManager : public input::InputManager
{
public:
    explicit InputManager(std::shared_ptr<droidinput::EventHubInterface> const& event_hub,
                          std::shared_ptr<InputThread> const& reader_thread);
    virtual ~InputManager();

    void start();
    void stop();

private:
    std::shared_ptr<droidinput::EventHubInterface> const event_hub;
    std::shared_ptr<InputThread> const reader_thread;
};
}
}
}

#endif // MIR_INPUT_INPUT_MANAGER
