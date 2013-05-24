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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_INPUT_ANDROID_INPUT_CONFIGURATION_H_
#define MIR_INPUT_ANDROID_INPUT_CONFIGURATION_H_

#include <utils/StrongPointer.h>

#include <memory>

namespace droidinput = android;

namespace android
{
class EventHubInterface;
class InputDispatcherInterface;
}

namespace mir
{
namespace surfaces
{
class InputRegistrar;
}
namespace shell
{
class InputTargeter;
}
namespace input
{
class InputTargets;
class InputManager;

namespace android
{
class InputThread;

class InputConfiguration
{
public:
    virtual ~InputConfiguration() {}

    // Perhaps these belong to a more general mi:: (rather than mia::) configuration interface.
    virtual std::shared_ptr<surfaces::InputRegistrar> the_input_registrar() = 0;
    virtual std::shared_ptr<shell::InputTargeter> the_input_targeter() = 0;
    virtual std::shared_ptr<input::InputManager> the_input_manager() = 0;
    
    virtual void set_input_targets(std::shared_ptr<input::InputTargets> const& targets) = 0;

protected:
    InputConfiguration() = default;
    InputConfiguration(InputConfiguration const&) = delete;
    InputConfiguration& operator=(InputConfiguration const&) = delete;
};
}
}
} // namespace mir

#endif // MIR_INPUT_ANDROID_INPUT_CONFIGURATION_H_
